#include <fuse_lowlevel.h>
#include <sys/stat.h>       // stat, S_IFDIR, S_IFREG

#include <cerrno>
#include <cstring>          // std::memset
#include <memory>           // std::make_unique
#include <string>
#include <utility>          // std::pair
#include <vector>

#include <state.hpp>
#include <util.hpp>         // xynta::exception_to_errno

void xynta_readdir(
    fuse_req_t req,
    fuse_ino_t ino,
    size_t size,
    off_t off,
    struct fuse_file_info* fi)
try {
    auto& dir = *reinterpret_cast<const xynta::dir*>(fi->fh);
    auto buf = std::make_unique<char[]>(size);
    char* out = buf.get();
    struct stat stat;
    std::memset(&stat, 0, sizeof stat);
    while (off < dir.size()) {
        stat.st_ino = ino + off;
        const auto& entry = dir[off++];
        if (entry.second) {
            stat.st_mode = S_IFDIR;
        } else {
            stat.st_mode = S_IFREG;
        }
        size_t entry_size = fuse_add_direntry(
            req,
            out,
            size,
            entry.first.c_str(),
            &stat,
            off);
        if (entry_size <= size) {
            out += entry_size;
            size -= entry_size;
        } else {
            break;
        }
    }
    fuse_reply_buf(req, buf.get(), out - buf.get());
} catch (...) {
    fuse_reply_err(req, xynta::exception_to_errno());
}

