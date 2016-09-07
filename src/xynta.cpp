#include <errno.h>
#include <fcntl.h>      // open
#include <fuse.h>
#include <string.h>     // strrchr
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
#include <vector>

#include "fs.hpp"
#include "util.hpp"

static const xynta::fs* fs;

static int xynta_getattr(const char* path, struct stat* stat) {
    int rc;
    if (path[1]) {
        try {
            std::string name{strrchr(path, '/') + 1};
            if (fs->is_file(name)) {
                rc = ::stat((fs->root + name).c_str(), stat);
                if (rc == 0) {
                    stat->st_mode &= ~0222;
                } else {
                    rc = -errno;
                }
            } else {
                memset(stat, 0, sizeof(struct stat));
                stat->st_mode = S_IFDIR | 0755;
                // TODO: fair links counting
                stat->st_nlink = 2;
                rc = 0;
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
            auto tags = xynta::split(path + 1, '/');
            auto iter = tags.begin();
            auto files{fs->files(*iter++)};
            std::vector<std::string> tmp;
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
                for (const auto& tag: fs->tags(file)) {
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
    try {
        std::string name{strrchr(path, '/') + 1};
        if (!fs->is_file(name)) {
            rc = -ENOENT;
        } else if ((fi->flags & 3) != O_RDONLY) {
            rc = -EACCES;
        } else {
            int fd = open((fs->root + name).c_str(), fi->flags);
            if (fd == -1) {
                rc = -errno;
            } else {
                rc = 0;
                fi->fh = fd;
            }
        }
    } catch (std::bad_alloc& e) {
        rc = -ENOMEM;
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
    struct fuse_operations ops = {};
    ops.getattr = xynta_getattr;
    ops.readdir = xynta_readdir;
    ops.open = xynta_open;
    ops.release = xynta_release;
    ops.read = xynta_read;
    argv[1] = argv[0];
    return fuse_main(argc - 1, argv + 1, &ops, 0);
}

