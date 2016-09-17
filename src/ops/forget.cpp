#include <fuse_lowlevel.h>

#include <state.hpp>
#include <util.hpp>         // xynta::exception_to_errno

void xynta_forget(fuse_req_t req, fuse_ino_t ino, unsigned long)
try {
    auto& state = *reinterpret_cast<xynta::state*>(fuse_req_userdata(req));
    if (state.is_folder_ino(ino)) {
        state.remove_folder(ino);
    } else {
        state.remove_file(ino);
    }
    fuse_reply_none(req);
} catch (...) {
    fuse_reply_err(req, xynta::exception_to_errno());
}

