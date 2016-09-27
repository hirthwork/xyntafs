#include "folder_node.hpp"

#include <fuse_lowlevel.h>  // fuse_ino_t, fuse_req_t, fuse_add_direntry
#include <sys/stat.h>       // stat, S_IFDIR, S_IFREG

#include <cstring>          // std::memset

#include <unordered_set>
#include <utility>          // std::move
#include <vector>

#include "fs.hpp"
#include "handle.hpp"

namespace xynta {

struct folder_handle: handle {
    const xynta::fs& fs;
    const std::vector<fuse_ino_t> entries;

    folder_handle(const xynta::fs& fs, std::vector<fuse_ino_t>&& entries)
        : fs(fs)
        , entries(std::move(entries))
    {
    }

    virtual size_t read(fuse_req_t req, char* buf, size_t size, off_t off)
        override
    {
        struct stat stat;
        std::memset(&stat, 0, sizeof stat);
        char* out = buf;
        while (static_cast<decltype(entries.size())>(off) < entries.size()) {
            auto ino = entries[off];
            stat.st_ino = ino;
            if (ino & 1) {
                stat.st_mode = S_IFDIR;
            } else {
                stat.st_mode = S_IFREG;
            }
            const char* name;
            switch (off++) {
                case 0:
                    name = ".";
                    break;
                case 1:
                    name = "..";
                    break;
                default:
                    if (ino & 1) {
                        name = fs.get_tag_name(ino).c_str();
                    } else {
                        name = fs.get_file_info(ino).filename.c_str();
                    }
                    break;
            }
            size_t out_size =
                fuse_add_direntry(req, out, size, name, &stat, off);
            if (out_size <= size) {
                out += out_size;
                size -= out_size;
            } else {
                break;
            }
        }
        return out - buf;
    }
};

void folder_node::getattr(struct stat* stat) const {
    stat->st_mode = S_IFDIR | 0555;
    // TODO: fair links counting
    stat->st_nlink = 2;
}

handle* folder_node::open(int) const {
    std::unordered_set<fuse_ino_t> tags;
    // dedup files tags
    for (const auto& file: files) {
        auto& file_tags = fs.get_file_info(file).tags;
        tags.insert(file_tags.begin(), file_tags.end());
    }
    // remove tags already presented in path
    for (auto& tag: path) {
        tags.erase(tag);
    }
    std::vector<fuse_ino_t> entries;
    entries.reserve(2 + files.size() + tags.size());
    entries.emplace_back(0);
    entries.emplace_back(0);
    entries.insert(entries.end(), tags.begin(), tags.end());
    entries.insert(entries.end(), files.begin(), files.end());
    return new folder_handle{fs, std::move(entries)};
}

}

