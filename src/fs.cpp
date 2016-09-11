#include <dirent.h>
#include <errno.h>
#include <string.h>     // strtok
#include <sys/stat.h>
#include <sys/types.h>  // DIR
#include <sys/xattr.h>  // getxattr

#include <algorithm>    // sort & unique & lower_bound
#include <memory>       // make_unique
#include <stdexcept>    // logic_error
#include <string>
#include <system_error>
#include <utility>      // move
#include <vector>

#include "fs.hpp"
#include "util.hpp"

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

static std::vector<std::string> load_tags(const char* path) {
    ssize_t size = getxattr(path, TAGS_ATTRIBUTE, 0, 0);
    while (true) {
        auto value = std::make_unique<char[]>(size + 1);
        ssize_t new_size = getxattr(path, TAGS_ATTRIBUTE, value.get(), size);
        if (new_size < 0) {
            int err = errno;
            if (err == ENODATA) {
                return {};
            } else {
                throw std::system_error(err, std::system_category());
            }
        } else if (new_size > size) {
            size = new_size;
        } else {
            value.get()[new_size] = 0;
            auto tags = xynta::split(value.get(), ' ');
            std::sort(tags.begin(), tags.end());
            tags.erase(std::unique(tags.begin(), tags.end()), tags.end());
            return tags;
        }
    }
}

static void insert(std::vector<std::string>& data, const std::string& str) {
    auto pos = std::lower_bound(data.begin(), data.end(), str);
    if (pos == data.end() || *pos != str) {
        data.insert(pos, str);
    }
}

xynta::fs::fs(std::string root) {
    root.push_back('/');
    process_dir(root);
    std::sort(all_files.begin(), all_files.end());
    for (const auto& tag: all_tags) {
        if (file_infos.find(tag) != file_infos.end()) {
            throw std::logic_error("Duplicated tag and file name: " + tag);
        }
    }
}

void xynta::fs::process_dir(const std::string& root) {
    dir d(root.c_str());
    while (struct dirent* dirent = d.next()) {
        if (*dirent->d_name != '.') {
            auto path = root + dirent->d_name;
            struct stat stat;
            if (::stat(path.c_str(), &stat)) {
                throw std::system_error(errno, std::system_category());
            } else if (stat.st_mode & S_IFREG) {
                process_file(std::move(path), dirent->d_name);
            } else if (stat.st_mode & S_IFDIR) {
                path.push_back('/');
                process_dir(path);
            } else {
                throw std::logic_error(
                    "Don't know how to process " + path
                    + " with flags " + std::to_string(stat.st_mode));
            }
        }
    }
}

void xynta::fs::process_file(std::string&& path, std::string&& filename) {
    all_files.push_back(filename);
    auto tags = load_tags(path.c_str());
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

