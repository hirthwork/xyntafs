#ifndef __XYNTA_FS_HPP__
#define __XYNTA_FS_HPP__

#include <set>
#include <string>
#include <system_error>
#include <unordered_map>

namespace xynta {

class fs {
    std::set<std::string> all_tags;
    std::set<std::string> all_files;
    std::unordered_map<std::string, std::set<std::string>> tag_files;
    std::unordered_map<std::string, std::set<std::string>> file_tags;

    static const std::set<std::string>& find(
        const std::unordered_map<std::string, std::set<std::string>>& map,
        const std::string& key)
    {
        auto iter = map.find(key);
        if (iter == map.end()) {
            throw std::system_error(
                std::make_error_code(std::errc::no_such_file_or_directory));
        } else {
            return iter->second;
        }
    }

public:
    const std::string root;

    fs(std::string&& root);

    const std::set<std::string>& tags() const {
        return all_tags;
    }

    const std::set<std::string>& files() const {
        return all_files;
    }

    const std::set<std::string>& tags(const std::string& file) const {
        return find(file_tags, file);
    }

    const std::set<std::string>& files(const std::string& tag) const {
        return find(tag_files, tag);
    }

    bool is_file(const std::string& file) const {
        return file_tags.find(file) != file_tags.end();
    }
};

}

#endif

