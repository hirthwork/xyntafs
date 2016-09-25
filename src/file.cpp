#include <fcntl.h>          // open
#include <unistd.h>         // close, pread

#include <cerrno>
#include <cstddef>          // std::size_t

#include <string>
#include <system_error>

#include "file.hpp"

xynta::file::file(const std::string& path, int flags)
    : fd(open(path.c_str(), flags))
{
    if (fd == -1) {
        throw std::system_error(errno, std::system_category());
    }
}

xynta::file::~file() {
    close(fd);
}

std::size_t xynta::file::read(char* buf, std::size_t size, std::size_t off) {
    char* out = buf;
    do {
        ssize_t read = pread(fd, out, size, off);
        if (read == -1) {
            throw std::system_error(errno, std::system_category());
        } else if (read) {
            out += read;
            off += read;
            size -= read;
        } else {
            break;
        }
    } while (size);
    return out - buf;
}

