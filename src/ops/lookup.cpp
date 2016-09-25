#include <fuse_lowlevel.h>
#include <sys/stat.h>

#include <cerrno>
#include <cstring>          // std::memset

#include <algorithm>        // std::set_intersection, std::binary_search
                            // std::min, std::lower_bound
#include <iterator>         // std::back_inserter
#include <type_traits>      // std::decay
#include <utility>          // std::move

#include <fs.hpp>
#include <util.hpp>         // xynta::exception_to_errno

void xynta_lookup(fuse_req_t req, fuse_ino_t parent, const char* psz)
try {
    auto& fs = *reinterpret_cast<xynta::fs*>(fuse_req_userdata(req));
    auto& parent_folder_node = fs.get_folder_node(parent);
    auto ino = fs.ino(psz);
    if (ino & 1) {
        // this is a tag
        // check for duplicated tags
        auto tag_pos = std::lower_bound(
            parent_folder_node.path.begin(),
            parent_folder_node.path.end(),
            ino);
        if (tag_pos != parent_folder_node.path.end() && *tag_pos == ino) {
            // duplicated tags in path are forbidden
            fuse_reply_err(req, ENOENT);
        } else {
            xynta::folder_node folder_node;
            folder_node.path.reserve(parent_folder_node.path.size() + 1);
            folder_node.path.insert(
                folder_node.path.end(),
                parent_folder_node.path.begin(),
                tag_pos);
            folder_node.path.push_back(ino);
            folder_node.path.insert(
                folder_node.path.end(),
                tag_pos,
                parent_folder_node.path.end());
            auto& tag_files = fs.get_files(ino);
            folder_node.files.reserve(
                std::min(parent_folder_node.files.size(), tag_files.size()));
            std::set_intersection(
                parent_folder_node.files.begin(),
                parent_folder_node.files.end(),
                tag_files.begin(),
                tag_files.end(),
                std::back_inserter(folder_node.files));
            if (folder_node.files.empty()) {
                // this folder will be empty, which is currently forbidden
                fuse_reply_err(req, ENOENT);
            } else {
                fs.store_folder(
                    std::move(folder_node),
                    [req] (fuse_ino_t folder_ino) {
                        struct fuse_entry_param entry;
                        std::memset(&entry, 0, sizeof entry);
                        entry.ino = folder_ino;
                        entry.attr.st_ino = folder_ino;
                        entry.attr.st_mode = S_IFDIR | 0555;
                        // TODO: fair links counting
                        entry.attr.st_nlink = 2;
                        return fuse_reply_entry(req, &entry) == 0;
                    });
            }
        }
    } else if (ino
        && std::binary_search(
            parent_folder_node.files.begin(),
            parent_folder_node.files.end(),
            ino))
    {
        // this is a regular file which belongs to parent directory
        struct fuse_entry_param entry;
        std::memset(&entry, 0, sizeof entry);
        if (stat(fs.get_file_info(ino).path.c_str(), &entry.attr)) {
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

