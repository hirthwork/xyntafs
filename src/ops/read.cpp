#include <fuse_lowlevel.h>
#include <unistd.h>         // pread

#include <cerrno>

#include <memory>           // std::make_unique

#include <util.hpp>         // xynta::exception_to_errno

void xynta_read(
    fuse_req_t req,
    fuse_ino_t,
    size_t size,
    off_t off,
    struct fuse_file_info* fi)
try {
    auto buf = std::make_unique<char[]>(size);
    char* out = buf.get();
    do {
        ssize_t read = pread(fi->fh, out, size, off);
        if (read == -1) {
            fuse_reply_err(req, errno);
            return;
        } else if (read) {
            out += read;
            off += read;
            size -= read;
        } else {
            break;
        }
    } while (size);
    fuse_reply_buf(req, buf.get(), out - buf.get());
} catch (...) {
    fuse_reply_err(req, xynta::exception_to_errno());
}

