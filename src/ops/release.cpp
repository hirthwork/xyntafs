#include <fuse_lowlevel.h>
#include <unistd.h>         // close

#include <cerrno>

void xynta_release(fuse_req_t req, fuse_ino_t, struct fuse_file_info* fi) {
    if (close(fi->fh)) {
        fuse_reply_err(req, errno);
    }
}

