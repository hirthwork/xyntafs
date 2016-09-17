#ifndef __XYNTA__STATE_HPP__
#define __XYNTA__STATE_HPP__

#include <fuse_lowlevel.h>

#include <mutex>            // std::unique_lock
#include <shared_mutex>
#include <string>
#include <system_error>
#include <unordered_map>
#include <vector>
#include <utility>          // std::move, std::pair

#include "fs.hpp"

namespace xynta {

// each directory entry has name and is_dir flag
typedef std::vector<std::pair<std::string, bool>> dir;

class state: public fs {
    std::unordered_map<fuse_ino_t, std::vector<std::string>> folders;
    std::unordered_map<fuse_ino_t, std::string> files;
    mutable std::shared_mutex folders_mutex;
    mutable std::shared_mutex files_mutex;
    fuse_ino_t folders_ino_counter;
    fuse_ino_t files_ino_counter;

    template <class M>
    const auto& find(const M& map, fuse_ino_t ino) const {
        auto iter = map.find(ino);
        if (iter == map.end()) {
            throw std::system_error(
                std::make_error_code(
                    std::errc::no_such_file_or_directory));
        } else {
            return iter->second;
        }
    }

public:
    state(std::string&& root)
        : fs{std::move(root)}
        , folders_ino_counter{1}
        , files_ino_counter{}
    {
    }

    state(const state&) = delete;
    state& operator =(const state&) = delete;

    bool is_folder_ino(fuse_ino_t ino) const {
        return ino & 1;
    }

    const std::vector<std::string>& folder_files(fuse_ino_t ino) const {
        if (ino == FUSE_ROOT_ID) {
            return fs::files();
        } else {
            std::shared_lock<std::shared_mutex> lock(folders_mutex);
            return find(folders, ino);
        }
    }

    const std::string& filename(fuse_ino_t ino) const {
        std::shared_lock<std::shared_mutex> lock(files_mutex);
        return find(files, ino);
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
    fuse_ino_t store_folder(std::vector<std::string>&& files) {
        std::unique_lock<std::shared_mutex> lock(folders_mutex);
        fuse_ino_t ino = folders_ino_counter += 2;
        folders.emplace(ino, std::move(files));
        return ino;
    }

    void remove_folder(fuse_ino_t ino) {
        std::unique_lock<std::shared_mutex> lock(folders_mutex);
        folders.erase(ino);
    }

    fuse_ino_t store_file(std::string&& filename) {
        std::unique_lock<std::shared_mutex> lock(files_mutex);
        fuse_ino_t ino = files_ino_counter += 2;
        files.emplace(ino, std::move(filename));
        return ino;
    }

    void remove_file(fuse_ino_t ino) {
        std::unique_lock<std::shared_mutex> lock(files_mutex);
        files.erase(ino);
    }
};

}

#endif

