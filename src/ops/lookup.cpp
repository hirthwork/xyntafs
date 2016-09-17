#include <fuse_lowlevel.h>
#include <sys/stat.h>

#include <cerrno>
#include <cstring>          // std::memset

#include <algorithm>        // std::set_intersection,min,binary_search
#include <iterator> 	    // std::back_inserter
#include <type_traits>      // std::decay
#include <utility>          // std::move

#include <fs.hpp>
#include <util.hpp>         // xynta::exception_to_errno

void xynta_lookup(fuse_req_t req, fuse_ino_t parent, const char* psz)
try {
    auto& fs = *reinterpret_cast<xynta::fs*>(fuse_req_userdata(req));
    auto& files = fs.folder_files(parent);
    auto ino = fs.ino(psz);
    if (ino & 1) {
        // this is a tag
        auto& tag_files = fs.files(ino);
        std::decay<decltype(tag_files)>::type result_files;
        result_files.reserve(std::min(files.size(), tag_files.size()));
        std::set_intersection(
            files.begin(), files.end(),
            tag_files.begin(), tag_files.end(),
            std::back_inserter(result_files));
        if (result_files.empty()) {
            // this folder will be empty, which is currently forbidden
            fuse_reply_err(req, ENOENT);
        } else {
            auto folder_ino = fs.store_folder(std::move(result_files));
            struct fuse_entry_param entry;
            std::memset(&entry, 0, sizeof entry);
            entry.ino = folder_ino;
            entry.attr.st_ino = folder_ino;
            entry.attr.st_mode = S_IFDIR | 0555;
            // TODO: fair links counting
            entry.attr.st_nlink = 2;
            // TODO: check return value, see fs::store_folder description
            fuse_reply_entry(req, &entry);
        }
    } else if (ino && binary_search(files.begin(), files.end(), ino)) {
        // this is a regular file which belongs to parent directory
        struct fuse_entry_param entry;
        std::memset(&entry, 0, sizeof entry);
        if (stat(fs.file_info(ino).path.c_str(), &entry.attr)) {
            fuse_reply_err(req, errno);
        } else {
            entry.ino = ino;
            entry.attr.st_ino = ino;
            entry.attr.st_mode &= ~0222;
            fuse_reply_entry(req, &entry);
        }
    } else {
        fuse_reply_err(req, ENOENT);
    }
} catch (...) {
    fuse_reply_err(req, xynta::exception_to_errno());
}

