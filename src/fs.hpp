#ifndef __XYNTA__FS_HPP__
#define __XYNTA__FS_HPP__

#include <fuse_lowlevel.h>  // fuse_ino_t

#include <memory>           // std::unique_ptr
#include <mutex>            // std::unique_lock
#include <shared_mutex>     // std::shared_mutex, std::shared_lock
#include <string>
#include <system_error>
#include <unordered_map>
#include <vector>
#include <utility>          // std::move

#include "node.hpp"

namespace xynta {

struct file_info {
    const std::string& filename;
    std::string path;
    std::vector<fuse_ino_t> tags;

    file_info() = default;
    file_info(file_info&&) = default;
    file_info(const file_info&) = delete;
    file_info& operator =(const file_info&) = delete;
};

class fs {
    std::unordered_map<std::string, fuse_ino_t> name_to_ino;
    std::unordered_map<fuse_ino_t, const std::string&> ino_to_tag;
    std::unordered_map<fuse_ino_t, file_info> ino_to_file;

    std::unordered_map<fuse_ino_t, std::vector<fuse_ino_t>> tag_files;

    std::unordered_map<fuse_ino_t, std::unique_ptr<node>> nodes;
    mutable std::shared_mutex mutex;
    fuse_ino_t folders_ino_counter;
    fuse_ino_t files_ino_counter;

    template <class M>
    static const auto& find(const M& map, fuse_ino_t ino) {
        auto iter = map.find(ino);
        if (iter == map.end()) {
            throw std::system_error(
                std::make_error_code(std::errc::no_such_file_or_directory));
        } else {
            return iter->second;
        }
    }

    void process_dir(
        const std::vector<fuse_ino_t>& tags,
        const std::string& root,
        std::vector<fuse_ino_t>& all_files);
    fuse_ino_t process_file(
        std::string&& filename,
        std::string&& path,
        std::vector<fuse_ino_t>&& tags);
    void load_tags(std::vector<fuse_ino_t>& tags, const char* path);
    fuse_ino_t add_tag(std::string&& tag);

public:
    fs(std::string&& root);

    fs(const fs&) = delete;
    fs& operator =(const fs&) = delete;

    template <class K>
    fuse_ino_t ino(const K& name) const {
        auto iter = name_to_ino.find(name);
        if (iter == name_to_ino.end()) {
            return {};
        } else {
            return iter->second;
        }
    }

    const file_info& get_file_info(fuse_ino_t ino) const {
        return find(ino_to_file, ino);
    }

    const std::string& get_tag_name(fuse_ino_t ino) const {
        return find(ino_to_tag, ino);
    }

    const std::vector<fuse_ino_t>& get_files(fuse_ino_t ino) const {
        return find(tag_files, ino);
    }

    const node& get_node(fuse_ino_t ino) const {
        std::shared_lock<std::shared_mutex> lock(mutex);
        return *find(nodes, ino);
    }

    template <class C>
    void store_node(std::unique_ptr<xynta::node>&& node, C callback) {
        std::unique_lock<std::shared_mutex> lock(mutex);
        fuse_ino_t ino = folders_ino_counter += 2;
        auto iter = nodes.emplace(ino, std::move(node));
        if (!callback(ino, *iter.first->second)) {
            nodes.erase(iter.first);
        }
    }

    void remove_node(fuse_ino_t ino) {
        std::unique_lock<std::shared_mutex> lock(mutex);
        nodes.erase(ino);
    }
};

}

#endif

