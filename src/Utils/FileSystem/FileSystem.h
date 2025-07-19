//
// Created by niffo on 7/16/2025.
//

#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include "Common/DefineWindows.h"
#include <string>
#include <vector>

typedef struct FILE_PATH_INFO
{
	std::string DirectoryNames;
	std::string FileName;
} DIRECTORY_AND_FILE_NAME;

class FileSystem
{
public:
	FileSystem() = default;
	~FileSystem() = default;

	FileSystem(const FileSystem&) = default;
	FileSystem(FileSystem&&) = default;
	FileSystem& operator=(const FileSystem&) = default;
	FileSystem& operator=(FileSystem&&) = default;

	bool OpenForRead(const std::string& path);
	bool OpenForWrite(const std::string& path);
	void Close();

	bool ReadBytes(void* dest, size_t size) const;
	void WriteBytes(const void* data, size_t size) const;

	bool ReadUInt32(uint32_t& value) const;
	void WriteUInt32(uint32_t value) const;

	bool ReadString(std::string& outStr) const;
	void WriteString(const std::string& str) const;
	void WritePlainText(const std::string& str) const;

	[[nodiscard]] uint64_t GetFileSize() const;
	[[nodiscard]] bool IsOpen() const;

	//~ Utility
	static bool IsPathExists(const std::wstring& path);
	static bool IsPathExists(const std::string& path);
	static bool IsDirectory(const std::string& path);
	static bool IsFile(const std::string& path);

	static bool CopyFiles(const std::string& source, const std::string& destination, bool overwrite = true);
	static bool MoveFiles(const std::string& source, const std::string& destination);
	static std::vector<char> ReadFromFile(const std::string& fileName);

	static DIRECTORY_AND_FILE_NAME SplitPathFile(const std::string& fullPath);

	template<typename... Args>
	static bool DeleteFiles(Args&&... args);

	template<typename... Args>
	static bool CreateDirectories(Args&&... args);

private:
	HANDLE m_hFile{ INVALID_HANDLE_VALUE };
	bool m_bReadMode{ false };
};

template<typename ...Args>
inline bool FileSystem::DeleteFiles(Args&& ...args)
{
	bool allSuccess = true;

	auto tryDelete = [&](const auto& path)
		{
			if (!DeleteFile(path.c_str())) allSuccess = false;
		};

	(tryDelete(std::forward<Args>(args)), ...); // Folding lets goo...
	return allSuccess;
}

template<typename ...Args>
inline bool FileSystem::CreateDirectories(Args&& ...args)
{
	bool allSuccess = true;

	auto tryCreate = [&](const auto& pathStr)
		{
			std::string current;
			for (size_t i = 0; i < pathStr.length(); ++i)
			{
				const wchar_t ch = pathStr[i];
				current += ch;

				if (ch == L'\\' || ch == L'/')
				{
					if (!current.empty() && !IsPathExists(current))
					{
						if (!CreateDirectory(current.c_str(),nullptr) && GetLastError() != ERROR_ALREADY_EXISTS)
						{
							allSuccess = false;
							return;
						}
					}
				}
			}

			// Final directory (if not ends with slash)
			if (!IsPathExists(current))
			{
				if (!CreateDirectory(current.c_str(), nullptr)
					&& GetLastError() != ERROR_ALREADY_EXISTS)
					allSuccess = false;
			}
		};

	(tryCreate(std::forward<Args>(args)), ...);
	return allSuccess;
}

#endif //FILESYSTEM_H
