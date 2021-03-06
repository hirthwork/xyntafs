#include "fs.hpp"

#include <dirent.h>         // opendir, readdir, closedir
#include <fuse_lowlevel.h>  // fuse_ino_t, FUSE_ROOT_ID
#include <stdlib.h>         // realpath, free
#include <sys/stat.h>
#include <sys/types.h>      // DIR
#include <sys/xattr.h>      // getxattr

#include <cerrno>

#include <algorithm>        // std::sort, std::uniqu
#include <memory>           // std::make_unique
#include <stdexcept>        // std::logic_error
#include <string>
#include <system_error>
#include <utility>          // std::move
#include <vector>

#include "file_node.hpp"
#include "folder_node.hpp"

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

xynta::fs::fs(std::string&& root)
    : folders_ino_counter{FUSE_ROOT_ID}
    , files_ino_counter{}
{
    root.push_back('/');
    std::vector<fuse_ino_t> all_files;
    process_dir({}, root, all_files);
    nodes.emplace(
        FUSE_ROOT_ID,
        std::make_unique<folder_node>(*this, std::move(all_files)));
    for (auto& tag: tag_files) {
        std::sort(tag.second.begin(), tag.second.end());
    }
}

void xynta::fs::process_dir(
    const std::vector<fuse_ino_t>& tags,
    const std::string& root,
    std::vector<fuse_ino_t>& all_files)
{
    dir d(root.c_str());
    while (struct dirent* dirent = d.next()) {
        if (*dirent->d_name != '.') {
            auto current_tags = tags;
            std::string name{dirent->d_name};
            char* fullpath = realpath((root + name).c_str(), nullptr);
            if (!fullpath) {
                throw std::system_error(errno, std::system_category());
            }
            std::string path{fullpath};
            free(fullpath);
            struct stat stat;
            if (::stat(path.c_str(), &stat)) {
                throw std::system_error(errno, std::system_category());
            } else if (stat.st_mode & S_IFREG) {
                auto pos = name.rfind('.');
                if (pos != std::string::npos && pos + 1 < name.size()) {
                    current_tags.emplace_back(add_tag(name.substr(pos + 1)));
                }
                all_files.push_back(
                    process_file(
                        std::move(name),
                        std::move(path),
                        std::move(current_tags)));
            } else if (stat.st_mode & S_IFDIR) {
                path.push_back('/');
                current_tags.emplace_back(add_tag(std::move(name)));
                process_dir(current_tags, path, all_files);
            } else {
                throw std::logic_error(
                    "Don't know how to process " + path
                    + " with flags " + std::to_string(stat.st_mode));
            }
        }
    }
}

fuse_ino_t xynta::fs::process_file(
    std::string&& filename,
    std::string&& path,
    std::vector<fuse_ino_t>&& tags)
{
    load_tags(tags, path.c_str());
    fuse_ino_t ino = files_ino_counter += 2;
    auto emplace_result = name_to_ino.emplace(std::move(filename), ino);
    if (!emplace_result.second) {
        if (emplace_result.first->second & 1) {
            throw std::logic_error("File and tag names collision: " + path);
        } else {
            throw std::logic_error(
                "Two files has same basename: " + path + " and "
                + ino_to_file.find(emplace_result.first->second)->second.path);
        }
    }
    std::sort(tags.begin(), tags.end());
    tags.erase(std::unique(tags.begin(), tags.end()), tags.end());
    for (const auto& tag: tags) {
        tag_files[tag].push_back(ino);
    }
    nodes.emplace(
        ino,
        std::make_unique<file_node>(
            ino_to_file.emplace(
                ino,
                file_info{
                    emplace_result.first->first,
                    std::move(path),
                    std::move(tags)}).first->second.path));
    return ino;
}

void xynta::fs::load_tags(std::vector<fuse_ino_t>& tags, const char* path) {
    ssize_t size = getxattr(path, TAGS_ATTRIBUTE, 0, 0);
    auto buf = std::make_unique<char[]>(size + 1);
    while (true) {
        ssize_t new_size = getxattr(path, TAGS_ATTRIBUTE, buf.get(), size);
        if (new_size < 0) {
            int err = errno;
            if (err == ENODATA) {
                return;
            } else {
                throw std::system_error(err, std::system_category());
            }
        } else if (new_size > size) {
            size = new_size;
            buf = std::make_unique<char[]>(size + 1);
        } else {
            buf.get()[new_size] = 0;
            break;
        }
    }

    std::string tmp;
    bool escaped = false;
    const char* str = buf.get();
    while (true) {
        char c = *str++;
        if (escaped) {
            switch (c) {
                case '\0':
                    tmp.push_back('\\');
                    tags.emplace_back(add_tag(std::move(tmp)));
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
                        tags.emplace_back(add_tag(std::move(tmp)));
                    }
                    return;
                case ' ':
                    if (!tmp.empty()) {
                        tags.emplace_back(add_tag(std::move(tmp)));
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

fuse_ino_t xynta::fs::add_tag(std::string&& tag) {
    folders_ino_counter += 2;
    auto emplace_result =
        name_to_ino.emplace(std::move(tag), folders_ino_counter);
    if (emplace_result.second) {
        ino_to_tag.emplace(folders_ino_counter, emplace_result.first->first);
    } else if ((emplace_result.first->second & 1) == 0) {
        throw std::logic_error(
            "Can't add tag. There is already file "
            + ino_to_file.find(emplace_result.first->second)->second.path
            + " present.");
    }
    return emplace_result.first->second;
}

