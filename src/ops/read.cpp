#include <fuse_lowlevel.h>

#include <memory>           // std::make_unique

#include <handle.hpp>
#include <util.hpp>         // xynta::exception_to_errno

void xynta_read(
    fuse_req_t req,
    fuse_ino_t,
    size_t size,
    off_t off,
    struct fuse_file_info* fi)
try {
    auto buf = std::make_unique<char[]>(size);
    xynta::handle* handle = reinterpret_cast<xynta::handle*>(fi->fh);
    size_t read = handle->read(req, buf.get(), size, off);
    fuse_reply_buf(req, buf.get(), read);
} catch (...) {
    fuse_reply_err(req, xynta::exception_to_errno());
}

