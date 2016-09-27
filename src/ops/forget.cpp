#include <fuse_lowlevel.h>

#include <fs.hpp>
#include <util.hpp>         // xynta::exception_to_errno

void xynta_forget(fuse_req_t req, fuse_ino_t ino, unsigned long)
try {
    auto& fs = *reinterpret_cast<xynta::fs*>(fuse_req_userdata(req));
    if (ino & 1) {
        fs.remove_node(ino);
    }
    fuse_reply_none(req);
} catch (...) {
    fuse_reply_err(req, xynta::exception_to_errno());
}

