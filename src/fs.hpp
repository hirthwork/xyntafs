#ifndef __XYNTA__FS_HPP__
#define __XYNTA__FS_HPP__

#include <fuse_lowlevel.h>

#include <cstddef>          // std::size_t

#include <mutex>            // std::unique_lock
#include <shared_mutex>
#include <string>
#include <system_error>
#include <unordered_map>
#include <vector>
#include <utility>          // std::move

namespace xynta {

typedef std::vector<fuse_ino_t> dir_listing;

struct file_info {
    const std::string& filename;
    const std::string path;
    const std::vector<fuse_ino_t> tags;
};

class fs {
    std::unordered_map<std::string, fuse_ino_t> name_to_ino;
    std::unordered_map<fuse_ino_t, const std::string&> ino_to_tag;
    std::unordered_map<fuse_ino_t, file_info> ino_to_file;

    std::vector<fuse_ino_t> all_files;
    std::unordered_map<fuse_ino_t, std::vector<fuse_ino_t>> tag_files;

    std::unordered_map<fuse_ino_t, std::vector<fuse_ino_t>> folders;
    mutable std::shared_mutex folders_mutex;
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
        const std::string& root);
    void process_file(
        std::string&& filename,
        std::string&& path,
        std::vector<fuse_ino_t>&& tags);
    void load_tags(std::vector<fuse_ino_t>& tags, const char* path);
    fuse_ino_t add_tag(std::string&& tag);

public:
    const std::size_t min_files;

    fs(std::string root, std::size_t min_files);

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

    const std::vector<fuse_ino_t>& get_folder_files(fuse_ino_t ino) const {
        if (ino == FUSE_ROOT_ID) {
            return all_files;
        } else {
            std::shared_lock<std::shared_mutex> lock(folders_mutex);
            return find(folders, ino);
        }
    }

    template <class C>
    void store_folder(std::vector<fuse_ino_t>&& files, C callback) {
        std::unique_lock<std::shared_mutex> lock(folders_mutex);
        fuse_ino_t ino = folders_ino_counter += 2;
        auto iter = folders.emplace(ino, std::move(files));
        if (!callback(ino)) {
            folders.erase(iter.first);
        }
    }

    void remove_folder(fuse_ino_t ino) {
        std::unique_lock<std::shared_mutex> lock(folders_mutex);
        folders.erase(ino);
    }
};

}

#endif

