#include <string.h>

#include <memory>
#include <set>
#include <string>

#include "util.hpp"

std::set<std::string> xynta::split(char* str, const char* delim) {
    std::set<std::string> tags;
    char* saveptr;
    char* token = strtok_r(str, delim, &saveptr);
    while (token) {
        tags.emplace(token);
        token = strtok_r(0, delim, &saveptr);
    }
    return tags;
}

std::set<std::string> xynta::split(const char* str, const char* delim) {
    size_t len = strlen(str) + 1;
    auto copy = std::make_unique<char[]>(len);
    memcpy(copy.get(), str, len);
    return split(copy.get(), delim);
}

