#include "file_helper.h"

#include <filesystem>
#include <fstream>

#include "data/enums.h"

namespace simulation
{

std::filesystem::path FileHelper::executable_dir_{};

FileHelper::FileHelper(std::shared_ptr<Logger> logger) : logger_(std::move(logger)) {}

bool FileHelper::DoesFileExist(const fs::path& file_path)
{
    return fs::exists(file_path);
}

bool FileHelper::DoesDirectoryExist(const fs::path& directory)
{
    return fs::is_directory(directory);
}

bool FileHelper::DoesDirectoryContainsFiles(const fs::path& directory) const
{
    return WalkFilesInDirectory_WithReturn(
        directory,
        [&](const fs::path&)
        {
            return true;
        });
}

std::string FileHelper::ReadAllContentFromFile(const fs::path& file_path) const
{
    std::string content;

    std::ifstream file(file_path, std::ios::in | std::ios::ate);
    if (file.is_open())
    {
        const std::streampos size = file.tellg();
        content.resize(static_cast<size_t>(size));
        file.seekg(0, std::ios::beg);
        file.read(content.data(), size);
    }
    else
    {
        logger_->Log(LogLevel::kErr, "Failed to open file [{}] for reading", file_path);
    }

    return content;
}

void FileHelper::WriteContentToFile(const fs::path& file_path, const std::string_view content)
{
    std::ofstream file(file_path, std::ios::out | std::ios::trunc);
    const auto length = static_cast<std::streamsize>(content.size());
    file.write(content.data(), length);
}

void FileHelper::WalkFilesInDirectory(
    const fs::path& dir_path,
    const std::function<void(const fs::path&)>& walk_function) const
{
    WalkFilesInDirectory_WithReturn(
        dir_path,
        [&](const fs::path& dir_entry)
        {
            walk_function(dir_entry);
            return false;  // do not stop iteration
        });
}

}  // namespace simulation
