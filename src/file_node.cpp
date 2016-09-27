#include "file_node.hpp"

#include <fcntl.h>          // open, O_ACCMODE, O_RDONLY
#include <fuse_lowlevel.h>  // fuse_req_t
#include <sys/stat.h>
#include <unistd.h>         // close, pread

#include <cerrno>

#include <system_error>

#include "handle.hpp"

namespace xynta {

struct file_handle: handle {
    const int fd;

    file_handle(const char* path, int flags)
        : fd(open(path, flags))
    {
        if (fd == -1) {
            throw std::system_error(errno, std::system_category());
        }
    }

    virtual ~file_handle() override {
        close(fd);
    }

    virtual size_t read(fuse_req_t, char* buf, size_t size, off_t off)
        override
    {
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
};

void file_node::getattr(struct stat* stat) const {
    if (::stat(path.c_str(), stat)) {
        throw std::system_error(errno, std::system_category());
    } else {
        stat->st_mode &= ~0222;
    }
}

handle* file_node::open(int flags) const {
    if ((flags & O_ACCMODE) == O_RDONLY) {
        return new file_handle(path.c_str(), flags);
    } else {
        throw std::system_error(
            std::make_error_code(std::errc::permission_denied));
    }
}

}

