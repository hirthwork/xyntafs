#include <fuse_lowlevel.h>

#include <cstdlib>          // std::free
#include <cstring>          // std::memset

#include "state.hpp"

void xynta_lookup(fuse_req_t req, fuse_ino_t parent, const char* name);
void xynta_forget(fuse_req_t req, fuse_ino_t ino, unsigned long nlookup);
void xynta_getattr(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info* fi);
void xynta_open(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info* fi);
void xynta_read(
    fuse_req_t req,
    fuse_ino_t ino,
    size_t size,
    off_t off,
    struct fuse_file_info* fi);
void xynta_release(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info* fi);
void xynta_opendir(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info* fi);
void xynta_readdir(
    fuse_req_t req,
    fuse_ino_t ino,
    size_t size,
    off_t off,
    struct fuse_file_info* fi);
void xynta_releasedir(
    fuse_req_t req,
    fuse_ino_t ino,
    struct fuse_file_info* fi);

int main(int argc, char* argv[]) {
    xynta::state state(argv[1]);
    struct fuse_lowlevel_ops ops;
    std::memset(&ops, 0, sizeof ops);
    ops.lookup = xynta_lookup;
    ops.forget = xynta_forget;
    ops.getattr = xynta_getattr;
    ops.open = xynta_open;
    ops.read = xynta_read;
    ops.release = xynta_release;
    ops.opendir = xynta_opendir;
    ops.readdir = xynta_readdir;
    ops.releasedir = xynta_releasedir;

    argv[1] = argv[0];
    struct fuse_args args = FUSE_ARGS_INIT(argc - 1, argv + 1);
    char* mountpoint;
    int multithreaded;
    int foreground;
    int rc = 1;
    if (!fuse_parse_cmdline(&args, &mountpoint, &multithreaded, &foreground)) {
        if (struct fuse_chan* ch = fuse_mount(mountpoint, &args)) {
            struct fuse_session* session =
                fuse_lowlevel_new(&args, &ops, sizeof ops, &state);
            if (session) {
                if (!fuse_set_signal_handlers(session)) {
                    fuse_session_add_chan(session, ch);
                    rc = fuse_daemonize(foreground);
                    if (!rc) {
                        if (multithreaded) {
                            rc = fuse_session_loop_mt(session);
                        } else {
                            rc = fuse_session_loop(session);
                        }
                    }
                    fuse_remove_signal_handlers(session);
                    fuse_session_remove_chan(ch);
                }
                fuse_session_destroy(session);
            }
            fuse_unmount(mountpoint, ch);
        }
        std::free(mountpoint);
    }
    fuse_opt_free_args(&args);
    return rc;
}

