#include <fuse_lowlevel.h>

#include <state.hpp>        // xynta::dir

void xynta_releasedir(fuse_req_t, fuse_ino_t, struct fuse_file_info* fi) {
    delete reinterpret_cast<xynta::dir*>(fi->fh);
}

