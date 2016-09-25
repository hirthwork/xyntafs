#ifndef __XYNTA__FILE_HPP__
#define __XYNTA__FILE_HPP__

#include <cstddef>          // std::size_t

#include <string>

#include <file_handle.hpp>

namespace xynta {

class file: public file_handle {
    const int fd;

public:
    file(const std::string& path, int flags);
    virtual ~file() override;
    virtual std::size_t read(
        char* buf,
        std::size_t size,
        std::size_t off)
        override;
};

}

#endif

