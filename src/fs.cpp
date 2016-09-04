#include <dirent.h>
#include <errno.h>
#include <string.h>     // strtok
#include <sys/types.h>  // DIR
#include <sys/xattr.h>  // getxattr

#include <memory>       // unique_ptr
#include <set>
#include <string>
#include <system_error>
#include <utility>      // move

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

static std::set<std::string> load_tags(const char* path) {
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
            return xynta::split(value.get(), " ");
        }
    }
}

xynta::fs::fs(std::string&& root)
    : root(root + '/')
{
    dir d(this->root.c_str());
    while (struct dirent* dirent = d.next()) {
        if (*dirent->d_name != '.') {
            std::string filename{dirent->d_name};
            all_files.insert(filename);
            std::set<std::string> tags =
                load_tags((this->root + filename).c_str());
            all_tags.insert(tags.begin(), tags.end());
            for (const auto& tag: tags) {
                tag_files[tag].insert(filename);
            }
            file_tags.emplace(std::move(filename), std::move(tags));
        }
    }

    for (const auto& tag: all_tags) {
        if (file_tags.find(tag) != file_tags.end()) {
            throw std::logic_error("Duplicated tag and file name: " + tag);
        }
    }
}

