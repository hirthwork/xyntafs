#ifndef __XYNTA__FILE_NODE_HPP__
#define __XYNTA__FILE_NODE_HPP__

#include <sys/stat.h>

#include <string>

#include "handle.hpp"
#include "node.hpp"

namespace xynta {

struct file_node: node {
    const std::string& path;

    file_node(const std::string& path)
        : path(path)
    {
    }

    file_node(file_node&& other)
        : path(other.path)
    {
    }

    file_node(const file_node&) = delete;
    file_node& operator =(const file_node&) = delete;

    virtual void getattr(struct stat* stat) const override;
    virtual handle* open(int flags) const override;
};

}

#endif

