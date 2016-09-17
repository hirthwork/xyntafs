#include <fuse_lowlevel.h>

#include <cerrno>
#include <cstddef>          // std::size_t
#include <memory>           // std::unique_ptr
#include <string>
#include <unordered_map>
#include <utility>          // std::pair
#include <vector>

#include <state.hpp>
#include <util.hpp>         // xynta::exception_to_errno

void xynta_opendir(fuse_req_t req, fuse_ino_t ino, struct fuse_file_info* fi)
try {
    auto& state = *reinterpret_cast<xynta::state*>(fuse_req_userdata(req));
    auto& files = state.folder_files(ino);
    std::unordered_map<std::string, std::size_t> tags(files.size() << 1);
    for (const auto& file: files) {
        for (const auto& tag: state.file_info(file).tags) {
            ++tags[tag];
        }
    }
    std::unique_ptr<xynta::dir> dir{new xynta::dir};
    dir->reserve(2 + files.size() + tags.size());
    // TODO: use two constant strings here
    dir->emplace_back(std::make_pair(".", true));
    dir->emplace_back(std::make_pair("..", true));
    // do not print tags list for a small number of files
    if (files.size() > 10) {
        for (const auto& tag: tags) {
            if (tag.second < files.size()) {
                dir->emplace_back(std::make_pair(tag.first, true));
            }
        }
    }
    for (const auto& file: files) {
        dir->emplace_back(std::make_pair(file, false));
    }
    fi->fh = reinterpret_cast<decltype(fi->fh)>(dir.get());
    if (!fuse_reply_open(req, fi)) {
        dir.release();
    }
} catch (...) {
    fuse_reply_err(req, xynta::exception_to_errno());
}

