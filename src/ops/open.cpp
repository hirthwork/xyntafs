#include <fcntl.h>          // open & O_RDONLY
#include <fuse_lowlevel.h>
#include <unistd.h>         // close

#include <cerrno>

#include <state.hpp>
#include <util.hpp>         // xynta::exception_to_errno

void xynta_open(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info* fi)
try {
    if ((fi->flags & 3) == O_RDONLY) {
        auto& state = *reinterpret_cast<xynta::state*>(fuse_req_userdata(req));
        const auto& file_info = state.file_info(state.filename(ino));
        int fd = open(file_info.path.c_str(), fi->flags);
        if (fd == -1) {
            fuse_reply_err(req, errno);
        } else {
            fi->fh = fd;
            if (fuse_reply_open(req, fi)) {
                close(fd);
            }
        }
    } else {
        fuse_reply_err(req, EACCES);
    }
} catch (...) {
    fuse_reply_err(req, xynta::exception_to_errno());
}
