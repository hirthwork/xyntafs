#include <fuse_lowlevel.h>

#include <fs.hpp>
#include <handle.hpp>
#include <util.hpp>         // xynta::exception_to_errno

void xynta_open(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info* fi)
try {
    auto& fs = *reinterpret_cast<xynta::fs*>(fuse_req_userdata(req));
    xynta::handle* handle = fs.get_node(ino).open(fi->flags);
    fi->fh = reinterpret_cast<decltype(fi->fh)>(handle);
    if (fuse_reply_open(req, fi)) {
        // reply_failed, free the data
        delete handle;
    }
} catch (...) {
    fuse_reply_err(req, xynta::exception_to_errno());
}

