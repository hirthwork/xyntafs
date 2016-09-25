#include <fuse_lowlevel.h>

#include <handle.hpp>

void xynta_release(fuse_req_t, fuse_ino_t, struct fuse_file_info* fi) {
    delete reinterpret_cast<xynta::handle*>(fi->fh);
}

