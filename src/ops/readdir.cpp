#include <fuse_lowlevel.h>
#include <sys/stat.h>       // stat, S_IFDIR, S_IFREG

#include <cstring>          // std::memset

#include <memory>           // std::make_unique

#include <fs.hpp>
#include <util.hpp>         // xynta::exception_to_errno

void xynta_readdir(
    fuse_req_t req,
    fuse_ino_t,
    size_t size,
    off_t off,
    struct fuse_file_info* fi)
try {
    auto& fs = *reinterpret_cast<xynta::fs*>(fuse_req_userdata(req));
    auto& dir = *reinterpret_cast<const xynta::dir_listing*>(fi->fh);
    auto buf = std::make_unique<char[]>(size);
    char* out = buf.get();
    struct stat stat;
    std::memset(&stat, 0, sizeof stat);
    while (static_cast<decltype(dir.size())>(off) < dir.size()) {
        auto ino = dir[off];
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
        size_t out_size = fuse_add_direntry(req, out, size, name, &stat, off);
        if (out_size <= size) {
            out += out_size;
            size -= out_size;
        } else {
            break;
        }
    }
    fuse_reply_buf(req, buf.get(), out - buf.get());
} catch (...) {
    fuse_reply_err(req, xynta::exception_to_errno());
}

