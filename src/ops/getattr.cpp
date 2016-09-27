#include <fuse_lowlevel.h>
#include <sys/stat.h>

#include <cstring>          // std::memset

#include <fs.hpp>
#include <util.hpp>         // xynta::exception_to_errno

void xynta_getattr(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info*)
try {
    struct stat stat;
    memset(&stat, 0, sizeof stat);
    auto& fs = *reinterpret_cast<xynta::fs*>(fuse_req_userdata(req));
    fs.get_node(ino).getattr(&stat);
    stat.st_ino = ino;
    fuse_reply_attr(req, &stat, 0);
} catch (...) {
    fuse_reply_err(req, xynta::exception_to_errno());
}

