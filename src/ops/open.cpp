#include <fcntl.h>          // O_RDONLY
#include <fuse_lowlevel.h>

#include <memory>           // std::unique_ptr

#include <file_handle.hpp>
#include <fs.hpp>
#include <handle.hpp>
#include <util.hpp>         // xynta::exception_to_errno

void xynta_open(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info* fi)
try {
    if ((fi->flags & O_ACCMODE) == O_RDONLY) {
        auto& fs = *reinterpret_cast<xynta::fs*>(fuse_req_userdata(req));
        std::unique_ptr<xynta::handle> file_handle{
            new xynta::file_handle{fs.get_file_info(ino).path, fi->flags}};
        fi->fh = reinterpret_cast<decltype(fi->fh)>(file_handle.get());
        if (!fuse_reply_open(req, fi)) {
            // reply successful, release the pointer as it already stored in fh
            file_handle.release();
        }
    } else {
        fuse_reply_err(req, EACCES);
    }
} catch (...) {
    fuse_reply_err(req, xynta::exception_to_errno());
}

