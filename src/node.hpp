#ifndef __XYNTA__NODE_HPP__
#define __XYNTA__NODE_HPP__

#include <sys/stat.h>

#include "handle.hpp"

namespace xynta {

struct node {
    node() = default;
    node(const node&) = delete;
    node& operator =(const node&) = delete;
    virtual ~node() = default;

    virtual void getattr(struct stat* stat) const = 0;
    virtual handle* open(int flags) const = 0;
};

}

#endif

