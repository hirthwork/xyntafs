#ifndef __XYNTA_FS_HPP__
#define __XYNTA_FS_HPP__

#include <string>
#include <system_error>
#include <unordered_map>
#include <vector>

namespace xynta {

struct file {
    const std::string path;
    const std::vector<std::string> tags;
};

class fs {
    std::vector<std::string> all_tags;
    std::vector<std::string> all_files;
    std::unordered_map<std::string, std::vector<std::string>> tag_files;
    std::unordered_map<std::string, file> file_infos;

    template <class M, class K>
    static const auto& find(const M& map, const K& key) {
        auto iter = map.find(key);
        if (iter == map.end()) {
            throw std::system_error(
                std::make_error_code(std::errc::no_such_file_or_directory));
        } else {
            return iter->second;
        }
    }

    void process_dir(
        const std::vector<std::string>& tags,
        const std::string& root);
    void process_file(
        std::vector<std::string>&& tags,
        std::string&& path,
        std::string&& filename);

public:
    fs(std::string root);

    const std::vector<std::string>& tags() const {
        return all_tags;
    }

    const std::vector<std::string>& files() const {
        return all_files;
    }

    template <class K>
    const file& file_info(const K& file) const {
	return find(file_infos, file);
    }

    template <class K>
    const std::vector<std::string>& files(const K& tag) const {
	return find(tag_files, tag);
    }

    bool is_tag(const std::string& tag) const {
        return tag_files.find(tag) != tag_files.end();
    }
};

}

#endif

