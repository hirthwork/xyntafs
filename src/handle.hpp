#ifndef __XYNTA__FILE_HANDLE_HPP__
#define __XYNTA__FILE_HANDLE_HPP__

#include <cstddef>          // std::size_t

namespace xynta {

struct handle {
    handle() = default;
    handle(const handle&) = delete;
    handle& operator =(const handle&) = delete;
    virtual ~handle() = default;

    // returns number of bytes read, throws on error
    // fewer bytes can be returned only at EOF
    virtual std::size_t read(char* buf, std::size_t size, std::size_t off) = 0;
};

}

#endif

