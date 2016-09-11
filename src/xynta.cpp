#include <errno.h>
#include <fcntl.h>      // open
#include <fuse.h>
#include <string.h>     // strrchr & memset
#include <sys/stat.h>
#include <unistd.h>     // close

#include <algorithm>    // set_intersection
#include <iostream>
#include <iterator> 	// inserter
#include <new>          // bad_alloc
#include <string>
#include <system_error>
#include <unordered_map>
#include <utility> 	// swap

#include "fs.hpp"

// TODO: SPEED: split into substring objects in order to avoid allocations
static std::vector<std::string> split(const char* str) {
    std::vector<std::string> tags;
    std::string tmp;
    while (true) {
        char c = *str++;
        switch (c) {
            case '\0':
                if (!tmp.empty()) {
                    tags.emplace_back(std::move(tmp));
                }
                return tags;
                break;
            case '/':
                if (!tmp.empty()) {
                    tags.emplace_back(std::move(tmp));
                    tmp.clear();
                }
                break;
            default:
                tmp.push_back(c);
                break;
        }
    }
}

static const xynta::fs* fs;

static int xynta_getattr(const char* path, struct stat* stat) {
    int rc;
    if (path[1]) {
        try {
            std::string name{strrchr(path, '/') + 1};
            if (fs->is_tag(name)) {
                memset(stat, 0, sizeof(struct stat));
                stat->st_mode = S_IFDIR | 0755;
                // TODO: fair links counting
                stat->st_nlink = 2;
                rc = 0;
            } else {
                rc = ::stat(fs->file_info(name).path.c_str(), stat);
                if (rc == 0) {
                    stat->st_mode &= ~0222;
                } else {
                    rc = -errno;
                }
            }
        } catch (std::system_error& e) {
            rc = -e.code().value();
        } catch (std::bad_alloc& e) {
            rc = -ENOMEM;
        }
    } else {
        memset(stat, 0, sizeof(struct stat));
        stat->st_mode = S_IFDIR | 0755;
        stat->st_nlink = 2 + fs->tags().size();
        rc = 0;
    }
    return rc;
}

static int xynta_readdir(
    const char* path,
    void* buf,
    fuse_fill_dir_t filler,
    off_t,
    struct fuse_file_info*)
{
    filler(buf, ".", 0, 0);
    filler(buf, "..", 0, 0);
    int rc = 0;
    if (path[1]) {
        try {
            auto tags = split(path + 1);
            auto iter = tags.begin();
            auto files = fs->files(*iter++);
            decltype(files) tmp;
            while (iter != tags.end()) {
                const auto& tag_files = fs->files(*iter++);
                std::set_intersection(
                    files.begin(), files.end(),
                    tag_files.begin(), tag_files.end(),
                    std::back_inserter(tmp));
                std::swap(files, tmp);
                tmp.clear();
            }
            std::unordered_map<std::string, size_t> tag_count;
            for (const auto& file: files) {
                filler(buf, file.c_str(), 0, 0);
                for (const auto& tag: fs->file_info(file).tags) {
                    ++tag_count[tag];
                }
            }
            for (const auto& tag: tag_count) {
                if (tag.second < files.size()) {
                    filler(buf, tag.first.c_str(), 0, 0);
                }
            }
        } catch (std::system_error& e) {
            rc = -e.code().value();
        } catch (std::bad_alloc& e) {
            rc = -ENOMEM;
        }
    } else {
        for (const auto& tag: fs->tags()) {
            filler(buf, tag.c_str(), 0, 0);
        }
        for (const auto& file: fs->files()) {
            filler(buf, file.c_str(), 0, 0);
        }
    }
    return rc;
}

static int xynta_open(const char* path, struct fuse_file_info* fi) {
    int rc;
    if ((fi->flags & 3) != O_RDONLY) {
        rc = -EACCES;
    } else {
        try {
            int fd = open(
                fs->file_info(strrchr(path, '/') + 1).path.c_str(),
                fi->flags);
            if (fd == -1) {
                rc = -errno;
            } else {
                rc = 0;
                fi->fh = fd;
            }
        } catch (std::system_error& e) {
            rc = -e.code().value();
        } catch (std::bad_alloc& e) {
            rc = -ENOMEM;
        }
    }
    return rc;
}

static int xynta_release(const char*, struct fuse_file_info* fi) {
    return -close(fi->fh);
}

static int xynta_read(
    const char*,
    char* buf,
    size_t size,
    off_t offset,
    struct fuse_file_info* fi)
{
    ssize_t read = pread(fi->fh, buf, size, offset);
    if (read == -1) {
        return -errno;
    } else {
        return read;
    }
}

int main(int argc, char* argv[]) {
    xynta::fs fs(argv[1]);
    ::fs = &fs;
    struct fuse_operations ops;
    memset(&ops, 0, sizeof ops);
    ops.getattr = xynta_getattr;
    ops.readdir = xynta_readdir;
    ops.open = xynta_open;
    ops.release = xynta_release;
    ops.read = xynta_read;
    argv[1] = argv[0];
    return fuse_main(argc - 1, argv + 1, &ops, 0);
}

