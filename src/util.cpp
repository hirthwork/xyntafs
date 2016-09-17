#include <cerrno>

#include <new>              // std::bad_alloc
#include <system_error>

#include "util.hpp"

int xynta::exception_to_errno() {
    try {
        throw;
    } catch (std::bad_alloc& e) {
        return ENOMEM;
    } catch (std::system_error& e) {
        return e.code().value();
    } catch (...) {
        return EINVAL;
    }
}
