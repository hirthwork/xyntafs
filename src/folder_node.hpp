#ifndef __XYNTA__FOLDER_NODE_HPP__
#define __XYNTA__FOLDER_NODE_HPP__

#include <fuse_lowlevel.h>  // fuse_ino_t
#include <sys/stat.h>

#include <vector>
#include <utility>          // std::move

#include "fs.hpp"
#include "handle.hpp"
#include "node.hpp"

namespace xynta {

struct folder_node: node {
    const xynta::fs& fs;
    std::vector<fuse_ino_t> path;
    std::vector<fuse_ino_t> files;

    folder_node(
        const xynta::fs& fs,
        std::vector<fuse_ino_t>&& files)
        : fs(fs)
        , files(std::move(files))
    {
    }

    folder_node(folder_node&& other)
        : fs(other.fs)
        , path(std::move(other.path))
        , files(std::move(other.files))
    {
    }

    folder_node(const folder_node&) = delete;
    folder_node& operator =(const folder_node&) = delete;

    virtual void getattr(struct stat* stat) const override;
    virtual handle* open(int flags) const override;
};

}

#endif

