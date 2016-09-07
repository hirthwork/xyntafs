#include <string>
#include <vector>

#include "util.hpp"

std::vector<std::string> xynta::split(const char* str, char delim) {
    std::vector<std::string> tags;
    std::string tmp;
    bool escaped = false;
    while (true) {
        char c = *str++;
        switch (c) {
            case 0:
                if (escaped) {
                    tmp.push_back('\\');
                }
                if (!tmp.empty()) {
                    tags.push_back(std::move(tmp));
                }
                return tags;
                break;
            case '\\':
                escaped = true;
                break;
            default:
                if (escaped) {
                    escaped = false;
                    tmp.push_back(c);
                } else if (c == delim) {
                    if (!tmp.empty()) {
                        tags.push_back(std::move(tmp));
                        tmp.clear();
                    }
                } else {
                    tmp.push_back(c);
                }
                break;
        }
    }
}

