#include <fuse_lowlevel.h>

#include <cstddef>          // std::size_t

#include <memory>           // std::unique_ptr
#include <unordered_map>

#include <fs.hpp>
#include <util.hpp>         // xynta::exception_to_errno

void xynta_opendir(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info* fi)
try {
    auto& fs = *reinterpret_cast<xynta::fs*>(fuse_req_userdata(req));
    auto& folder_info = fs.get_folder_info(ino);
    std::unordered_map<fuse_ino_t, std::size_t> tags{
        folder_info.files.size() << 1};
    for (const auto& file: folder_info.files) {
        for (const auto& tag: fs.get_file_info(file).tags) {
            ++tags[tag];
        }
    }
    std::unique_ptr<xynta::dir_listing> dir{new xynta::dir_listing};
    dir->reserve(2 + folder_info.files.size() + tags.size());
    dir->emplace_back(ino);
    dir->emplace_back(ino);
    // do not print tags list for a small number of files
    if (folder_info.files.size() > fs.min_files) {
        for (const auto& tag: tags) {
            if (tag.second < folder_info.files.size()) {
                dir->emplace_back(tag.first);
            }
        }
    }
    dir->insert(
        dir->end(),
        folder_info.files.begin(),
        folder_info.files.end());
    fi->fh = reinterpret_cast<decltype(fi->fh)>(dir.get());
    if (!fuse_reply_open(req, fi)) {
        dir.release();
    }
} catch (...) {
    fuse_reply_err(req, xynta::exception_to_errno());
}

