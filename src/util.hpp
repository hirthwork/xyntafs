#ifndef __XYNTA_UTIL_HPP__
#define __XYNTA_UTIL_HPP__

#include <set>
#include <string>

namespace xynta {

std::set<std::string> split(char* str, const char* delim);
std::set<std::string> split(const char* str, const char* delim);

}

#endif

