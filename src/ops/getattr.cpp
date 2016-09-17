#include <fuse_lowlevel.h>
#include <sys/stat.h>

#include <cerrno>
#include <cstring>          // std::memset

#include <state.hpp>
#include <util.hpp>         // xynta::exception_to_errno

void xynta_getattr(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info*)
try {
    auto& state = *reinterpret_cast<xynta::state*>(fuse_req_userdata(req));
    struct stat stat;
    memset(&stat, 0, sizeof stat);
    if (state.is_folder_ino(ino)) {
        stat.st_ino = ino;
        stat.st_mode = S_IFDIR | 0555;
        // TODO: fair links counting
        stat.st_nlink = 2;
        fuse_reply_attr(req, &stat, 0);
    } else {
        const auto& file_info = state.file_info(state.filename(ino));
        if (::stat(file_info.path.c_str(), &stat)) {
            fuse_reply_err(req, errno);
        } else {
            stat.st_ino = ino;
            stat.st_mode &= ~0222;
            fuse_reply_attr(req, &stat, 0);
        }
    }
} catch (...) {
    fuse_reply_err(req, xynta::exception_to_errno());
}

