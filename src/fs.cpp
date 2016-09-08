#include <dirent.h>
#include <errno.h>
#include <string.h>     // strtok
#include <sys/types.h>  // DIR
#include <sys/xattr.h>  // getxattr

#include <algorithm>    // sort & unique & lower_bound
#include <iterator>     // move_iterator
#include <memory>       // unique_ptr
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

xynta::fs::fs(std::string&& root)
    : root(root + '/')
{
    dir d(this->root.c_str());
    while (struct dirent* dirent = d.next()) {
        if (*dirent->d_name != '.') {
            std::string filename{dirent->d_name};
            insert(all_files, filename);
            auto tags = load_tags((this->root + filename).c_str());
            for (const auto& tag: tags) {
                insert(all_tags, tag);
                insert(tag_files[tag], filename);
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

