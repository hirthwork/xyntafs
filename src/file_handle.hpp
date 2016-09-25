#ifndef __XYNTA__FILE_HPP__
#define __XYNTA__FILE_HPP__

#include <cstddef>          // std::size_t

#include <string>

#include <handle.hpp>

namespace xynta {

class file_handle: public handle {
    const int fd;

public:
    file_handle(const std::string& path, int flags);
    virtual ~file_handle() override;
    virtual std::size_t read(
        char* buf,
        std::size_t size,
        std::size_t off)
        override;
};

}

#endif

