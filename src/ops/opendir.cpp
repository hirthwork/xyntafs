#include <fuse_lowlevel.h>

#include <memory>           // std::unique_ptr
#include <unordered_set>

#include <fs.hpp>
#include <util.hpp>         // xynta::exception_to_errno

void xynta_opendir(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info* fi)
try {
    auto& fs = *reinterpret_cast<xynta::fs*>(fuse_req_userdata(req));
    auto& folder_info = fs.get_folder_info(ino);
    std::unordered_set<fuse_ino_t> tags;
    // dedup files tags
    for (const auto& file: folder_info.files) {
        auto& file_tags = fs.get_file_info(file).tags;
        tags.insert(file_tags.begin(), file_tags.end());
    }
    // remove tags already presented in path
    for (auto& tag: folder_info.path) {
        tags.erase(tag);
    }
    std::unique_ptr<xynta::dir_listing> dir{new xynta::dir_listing};
    dir->reserve(2 + folder_info.files.size() + tags.size());
    dir->emplace_back(ino);
    dir->emplace_back(ino);
    dir->insert(dir->end(), tags.begin(), tags.end());
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

