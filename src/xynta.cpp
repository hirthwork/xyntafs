#include <fuse_lowlevel.h>
#include <getopt.h>

#include <cstddef>          // std::size_t
#include <cstdlib>          // std::free

#include <cstring>          // std::memset
#include <iostream>         // std::cout, std::cerr
#include <string>           // std::string, std::stoul
#include <utility>          // std::move

#include "fs.hpp"
#include "util.hpp"         // xynta::exception_to_errno

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

void usage(const char* argv0, std::ostream& out) {
    out << "Usage: " << argv0
        << " --data-dir=<data-dir> [--min-files=<N>] [<fuse_opts>...] "
        << "<mountpoint>"
        << std::endl
        << "   or: " << argv0 << " --help"
        << std::endl
        << "Mount <data-dir> as tagged file system to <mountpoint>."
        << std::endl
        << std::endl
        << "Options:"
        << std::endl
        << "  -d, --data-dir=<data-dir>     Directory with files and tags to "
        << "mount."
        << std::endl
        << "  -h, --help                    Give this help list."
        << std::endl;
}

int main(int argc, char* argv[])
try {
    std::string data_dir;
    struct option opts[] = {
        {"help",        no_argument,        nullptr, 'h'},
        {"data-dir",    required_argument,  nullptr, 'd'},
        {nullptr,       0,                  nullptr,  0 }
    };
    int opt;
    while ((opt = getopt_long(argc, argv, "hd:", opts, nullptr)) != -1) {
        switch (opt) {
            case 'h':
                usage(argv[0], std::cout);
                return 0;
            case 'd':
                data_dir = optarg;
                break;
            default:
                std::cerr << "Unrecognized option '" << (char) opt << '\''
                    << std::endl;
                usage(argv[0], std::cerr);
                return 1;
        }
    }

    if (data_dir.empty()) {
        std::cerr << "Error: data-dir is not set" << std::endl;
        usage(argv[0], std::cerr);
        return 1;
    }

    xynta::fs fs(std::move(data_dir));
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

    argv[optind - 1] = argv[0];
    struct fuse_args args =
        FUSE_ARGS_INIT(argc - optind + 1, argv + optind - 1);
    char* mountpoint;
    int multithreaded;
    int foreground;
    int rc = 1;
    if (!fuse_parse_cmdline(&args, &mountpoint, &multithreaded, &foreground)) {
        if (struct fuse_chan* ch = fuse_mount(mountpoint, &args)) {
            struct fuse_session* session =
                fuse_lowlevel_new(&args, &ops, sizeof ops, &fs);
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
} catch (std::exception& e) {
    std::cerr << e.what() << std::endl;
    return xynta::exception_to_errno();
}

