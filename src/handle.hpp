#ifndef __XYNTA__FILE_HANDLE_HPP__
#define __XYNTA__FILE_HANDLE_HPP__

#include <fuse_lowlevel.h>  // fuse_req_t

namespace xynta {

struct handle {
    handle() = default;
    handle(const handle&) = delete;
    handle& operator =(const handle&) = delete;
    virtual ~handle() = default;

    // returns number of bytes read, throws on error
    // fewer bytes can be returned only at EOF
    virtual size_t read(fuse_req_t req, char* buf, size_t size, off_t off) = 0;
};

}

#endif

