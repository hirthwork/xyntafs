#ifndef __XYNTA__FS_HPP__
#define __XYNTA__FS_HPP__

#include <fuse_lowlevel.h>

#include <mutex>            // std::unique_lock
#include <shared_mutex>
#include <string>
#include <system_error>
#include <unordered_map>
#include <vector>
#include <utility>          // std::move

namespace xynta {

typedef std::vector<fuse_ino_t> dir_listing;

struct file {
    const std::string& filename;
    const std::string path;
    const std::vector<fuse_ino_t> tags;
};

class fs {
    std::unordered_map<std::string, fuse_ino_t> name_to_ino;
    std::unordered_map<fuse_ino_t, const std::string&> ino_to_tag;
    std::unordered_map<fuse_ino_t, file> ino_to_file;

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
    fs(std::string&& root);

    fs(const fs&) = delete;
    fs& operator =(const fs&) = delete;

    const std::vector<fuse_ino_t>& files() const {
        return all_files;
    }

    template <class K>
    fuse_ino_t ino(const K& name) const {
        auto iter = name_to_ino.find(name);
        if (iter == name_to_ino.end()) {
            return {};
        } else {
            return iter->second;
        }
    }

    const file& file_info(fuse_ino_t ino) const {
        return find(ino_to_file, ino);
    }

    const std::string& tag_name(fuse_ino_t ino) const {
        return find(ino_to_tag, ino);
    }

    const std::vector<fuse_ino_t>& files(fuse_ino_t ino) const {
        return find(tag_files, ino);
    }

    const std::vector<fuse_ino_t>& folder_files(fuse_ino_t ino) const {
        if (ino == FUSE_ROOT_ID) {
            return all_files;
        } else {
            std::shared_lock<std::shared_mutex> lock(folders_mutex);
            return find(folders, ino);
        }
    }

    // § 23.2.5.1/1
    // `erase(k)' does not throw an exception unless that exception is thrown
    // by the container's `Compare' object.
    //
    // Still exception can be thrown by lock, so in order to perform proper
    // clean up, mutex ownership must be taken exactly once, so new inode can
    // be safely removed from folders and counter can be decremented.
    //
    // This could lead to serious performance penalty, so probably
    // unordered_map should be distributed among several independent maps.
    fuse_ino_t store_folder(std::vector<fuse_ino_t>&& files) {
        std::unique_lock<std::shared_mutex> lock(folders_mutex);
        fuse_ino_t ino = folders_ino_counter += 2;
        folders.emplace(ino, std::move(files));
        return ino;
    }

    void remove_folder(fuse_ino_t ino) {
        std::unique_lock<std::shared_mutex> lock(folders_mutex);
        folders.erase(ino);
    }
};

}

#endif

