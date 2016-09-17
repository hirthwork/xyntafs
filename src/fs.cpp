#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>  // DIR
#include <sys/xattr.h>  // getxattr

#include <cerrno>

#include <algorithm>    // std::sort, std::unique, std::lower_bound
#include <memory>       // std::make_unique
#include <stdexcept>    // std::logic_error
#include <string>
#include <system_error>
#include <utility>      // std::move
#include <vector>

#include "fs.hpp"

#define TAGS_ATTRIBUTE "user.xynta.tags"

class dir {
    DIR* const d;

public:
    dir(const char* name)
        : d(opendir(name))
    {
        if (!d) {
            throw std::system_error(errno, std::system_category());
        }
    }

    ~dir() {
        closedir(d);
    }

    struct dirent* next() {
        return readdir(d);
    }
};

static void split(std::vector<std::string>& tags, const char* str) {
    std::string tmp;
    bool escaped = false;
    while (true) {
        char c = *str++;
        if (escaped) {
            switch (c) {
                case '\0':
                    tmp.push_back('\\');
                    tags.emplace_back(std::move(tmp));
                    return;
                case ' ':
                    tmp.push_back(' ');
                    break;
                case '\\':
                    tmp.push_back('\\');
                    break;
                default:
                    tmp.push_back('\\');
                    tmp.push_back(c);
                    break;
            }
            escaped = false;
        } else {
            switch (c) {
                case '\0':
                    if (!tmp.empty()) {
                        tags.emplace_back(std::move(tmp));
                    }
                    return;
                case ' ':
                    if (!tmp.empty()) {
                        tags.emplace_back(std::move(tmp));
                        tmp.clear();
                    }
                    break;
                case '\\':
                    escaped = true;
                    break;
                default:
                    tmp.push_back(c);
                    break;
            }
        }
    }
}

static void load_tags(std::vector<std::string>& tags, const char* path) {
    ssize_t size = getxattr(path, TAGS_ATTRIBUTE, 0, 0);
    while (true) {
        auto value = std::make_unique<char[]>(size + 1);
        ssize_t new_size = getxattr(path, TAGS_ATTRIBUTE, value.get(), size);
        if (new_size < 0) {
            int err = errno;
            if (err == ENODATA) {
                return;
            } else {
                throw std::system_error(err, std::system_category());
            }
        } else if (new_size > size) {
            size = new_size;
        } else {
            value.get()[new_size] = 0;
            split(tags, value.get());
            std::sort(tags.begin(), tags.end());
            tags.erase(std::unique(tags.begin(), tags.end()), tags.end());
            return;
        }
    }
}

static void insert(std::vector<std::string>& data, const std::string& str) {
    auto pos = std::lower_bound(data.begin(), data.end(), str);
    if (pos == data.end() || *pos != str) {
        data.insert(pos, str);
    }
}

xynta::fs::fs(std::string&& root) {
    root.push_back('/');
    process_dir({}, root);
    std::sort(all_files.begin(), all_files.end());
    for (const auto& tag: all_tags) {
        if (file_infos.find(tag) != file_infos.end()) {
            throw std::logic_error("Duplicated tag and file name: " + tag);
        }
    }
}

void xynta::fs::process_dir(
    const std::vector<std::string>& tags,
    const std::string& root)
{
    dir d(root.c_str());
    while (struct dirent* dirent = d.next()) {
        if (*dirent->d_name != '.') {
            auto current_tags = tags;
            std::string name{dirent->d_name};
            auto path = root + name;
            struct stat stat;
            if (::stat(path.c_str(), &stat)) {
                throw std::system_error(errno, std::system_category());
            } else if (stat.st_mode & S_IFREG) {
                auto pos = name.rfind('.');
                if (pos != std::string::npos && pos + 1 < name.size()) {
                    current_tags.emplace_back(name.substr(pos + 1));
                }
                process_file(
                    std::move(current_tags),
                    std::move(path),
                    std::move(name));
            } else if (stat.st_mode & S_IFDIR) {
                path.push_back('/');
                current_tags.emplace_back(std::move(name));
                process_dir(current_tags, path);
            } else {
                throw std::logic_error(
                    "Don't know how to process " + path
                    + " with flags " + std::to_string(stat.st_mode));
            }
        }
    }
}

void xynta::fs::process_file(
    std::vector<std::string>&& tags,
    std::string&& path,
    std::string&& filename)
{
    all_files.push_back(filename);
    load_tags(tags, path.c_str());
    for (const auto& tag: tags) {
        insert(all_tags, tag);
        insert(tag_files[tag], filename);
    }
    auto emplace_result = file_infos.emplace(
        std::move(filename),
        file{path, std::move(tags)});
    if (!emplace_result.second) {
        throw std::logic_error(
            "File names collision: " + path
            + " and " + emplace_result.first->second.path);
    }
}

