#pragma once

#include <filesystem>
#include <string>

#include "utility/logger.h"

namespace fs = std::filesystem;

namespace simulation
{

class FileHelper
{
public:
    explicit FileHelper(std::shared_ptr<Logger> logger);

    static bool DoesFileExist(const fs::path& file_path);
    static bool DoesDirectoryExist(const fs::path& directory);
    bool DoesDirectoryContainsFiles(const fs::path& directory) const;

    std::string ReadAllContentFromFile(const fs::path& file_path) const;

    static void WriteContentToFile(const fs::path& file_path, std::string_view content);

    void WalkFilesInDirectory(const fs::path& dir_path, const std::function<void(const fs::path&)>& walk_function)
        const;

    // Callback signature: bool(const fs::path&).
    // Return true from callback means stop iteration and return true from the method
    template <typename Callback>
    bool WalkFilesInDirectory_WithReturn(const fs::path& path, Callback&& callback) const
    {
        if (!DoesDirectoryExist(path))
        {
            logger_->LogErr("Folder does not exist: {}", path);
            return false;
        }

        for (const auto& dir_entry : fs::recursive_directory_iterator(path))
        {
            // Walk only files
            if (!dir_entry.is_directory())
            {
                logger_->LogDebug("WalkFilesInDirectory_WithReturn - File = {}", dir_entry.path());
                if (callback(dir_entry.path()))
                {
                    return true;
                }
            }
        }

        return false;
    }

    static void SetExecutableDirectory(const std::filesystem::path& path)
    {
        executable_dir_ = path;
    }

    static const std::filesystem::path& GetExecutableDirectory()
    {
        return executable_dir_;
    }

private:
    std::shared_ptr<Logger> logger_;

    static std::filesystem::path executable_dir_;
};

}  // namespace simulation
