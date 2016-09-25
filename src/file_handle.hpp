#ifndef __XYNTA__FILE_HANDLE_HPP__
#define __XYNTA__FILE_HANDLE_HPP__

#include <cstddef>          // std::size_t

namespace xynta {

struct file_handle {
    file_handle() = default;
    file_handle(const file_handle&) = delete;
    file_handle& operator =(const file_handle&) = delete;
    virtual ~file_handle() = default;

    // returns number of bytes read, throws on error
    // fewer bytes can be returned only at EOF
    virtual std::size_t read(char* buf, std::size_t size, std::size_t off) = 0;
};

}

#endif

