#include <fuse_lowlevel.h>
#include <sys/stat.h>

#include <cerrno>
#include <cstring>          // std::memset

#include <algorithm>        // std::set_intersection,min,binary_search
#include <iterator> 	    // std::back_inserter
#include <string>
#include <type_traits>      // std::decay
#include <utility>          // std::move

#include <fs.hpp>
#include <state.hpp>
#include <util.hpp>         // xynta::exception_to_errno

void xynta_lookup(fuse_req_t req, fuse_ino_t parent, const char* psz)
try {
    auto& state = *reinterpret_cast<xynta::state*>(fuse_req_userdata(req));
    xynta::fs& fs = state;
    auto& files = state.folder_files(parent);
    std::string name{psz};
    if (state.is_tag(name)) {
        auto& tag_files = fs.files(name);
        std::decay<decltype(tag_files)>::type result_files;
        result_files.reserve(std::min(files.size(), tag_files.size()));
        std::set_intersection(
            files.begin(), files.end(),
            tag_files.begin(), tag_files.end(),
            std::back_inserter(result_files));
        if (result_files.empty()) {
            fuse_reply_err(req, ENOENT);
        } else {
            auto ino = state.store_folder(std::move(result_files));
            struct fuse_entry_param entry;
            std::memset(&entry, 0, sizeof entry);
            entry.ino = ino;
            entry.attr.st_ino = ino;
            entry.attr.st_mode = S_IFDIR | 0555;
            // TODO: fair links counting
            entry.attr.st_nlink = 2;
            // TODO: check return value, see state::store_folder description
            fuse_reply_entry(req, &entry);
        }
    } else if (binary_search(files.begin(), files.end(), name)) {
        struct fuse_entry_param entry;
        std::memset(&entry, 0, sizeof entry);
        if (stat(fs.file_info(name).path.c_str(), &entry.attr)) {
            fuse_reply_err(req, errno);
        } else {
            auto ino = state.store_file(std::move(name));
            entry.ino = ino;
            entry.attr.st_ino = ino;
            entry.attr.st_mode &= ~0222;
            // TODO: check return value, see state::store_folder description
            fuse_reply_entry(req, &entry);
        }
    } else {
        fuse_reply_err(req, ENOENT);
    }
} catch (...) {
    fuse_reply_err(req, xynta::exception_to_errno());
}

