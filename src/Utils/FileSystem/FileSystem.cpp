//
// Created by niffo on 7/16/2025.
//

#include "FileSystem.h"
#include "ExceptionHandler/IException.h"
#include <ostream>

bool FileSystem::OpenForRead(const std::string& path)
{
	m_hFile = CreateFile(
		path.c_str(),
		GENERIC_READ,
		FILE_SHARE_READ,
		nullptr,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		nullptr
	);

	m_bReadMode = true;
	return m_hFile != INVALID_HANDLE_VALUE;
}

bool FileSystem::OpenForWrite(const std::string& path)
{
	//~ Separate File and Directory
	auto [DirectoryNames, FileName] = SplitPathFile(path);

	//~ Create Directory
	CreateDirectories(DirectoryNames);

	m_hFile = CreateFile(
		path.c_str(),
		GENERIC_WRITE,
		0,
		nullptr,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		nullptr
	);

	m_bReadMode = false;
	return m_hFile != INVALID_HANDLE_VALUE;
}

void FileSystem::Close()
{
	if (m_hFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_hFile);
		m_hFile = INVALID_HANDLE_VALUE;
		m_bReadMode = false;
	}
}

bool FileSystem::ReadBytes(void* dest, const size_t size) const
{
	if (!m_bReadMode || m_hFile == INVALID_HANDLE_VALUE) return false;

	DWORD bytesRead = 0;
	return ReadFile(m_hFile, dest, static_cast<DWORD>(size), &bytesRead, nullptr) && bytesRead == size;
}

void FileSystem::WriteBytes(const void* data, size_t size) const
{
	if (m_bReadMode || m_hFile == INVALID_HANDLE_VALUE) return;

	DWORD bytesWritten = 0;
	WriteFile(m_hFile, data, static_cast<DWORD>(size), &bytesWritten, nullptr) && bytesWritten == size;
}

bool FileSystem::ReadUInt32(uint32_t& value) const
{
	return ReadBytes(&value, sizeof(uint32_t));
}

void FileSystem::WriteUInt32(uint32_t value) const
{
	WriteBytes(&value, sizeof(uint32_t));
}

bool FileSystem::ReadString(std::string& outStr) const
{
	uint32_t len;
	if (!ReadUInt32(len)) return false;

	std::string buffer(len, '\0');
	if (!ReadBytes(buffer.data(), len)) return false;

	outStr = std::move(buffer);

	return true;
}

void FileSystem::WriteString(const std::string& str) const
{
	const auto len = static_cast<uint32_t>(str.size());
	WriteUInt32(len);
	WriteBytes(str.data(), len);
}

void FileSystem::WritePlainText(const std::string& str) const
{
	if (m_bReadMode || m_hFile == INVALID_HANDLE_VALUE) return;

	DWORD bytesWritten = 0;
	const std::string line = str + "\n";
	WriteFile(m_hFile, line.c_str(), static_cast<DWORD>(line.size()), &bytesWritten, nullptr);
}

uint64_t FileSystem::GetFileSize() const
{
	if (m_hFile == INVALID_HANDLE_VALUE) return 0;

	LARGE_INTEGER size{};
	if (!::GetFileSizeEx(m_hFile, &size)) return 0;

	return static_cast<uint64_t>(size.QuadPart);
}

bool FileSystem::IsOpen() const
{
	return m_hFile != INVALID_HANDLE_VALUE;
}

bool FileSystem::IsPathExists(const std::wstring& path)
{
	const auto cpath = std::string(path.begin(), path.end());
	const DWORD attr = GetFileAttributes(cpath.c_str());
	return (attr != INVALID_FILE_ATTRIBUTES);
}

DIRECTORY_AND_FILE_NAME FileSystem::SplitPathFile(const std::string& fullPath)
{
	// Supports both '/' and '\\'
	const size_t lastSlash = fullPath.find_last_of("/\\");
	if (lastSlash == std::string::npos)
	{
		// No folder, only filename
		return { "", fullPath };
	}

	return {
		fullPath.substr(0, lastSlash),
		fullPath.substr(lastSlash + 1)
	};
}

bool FileSystem::IsPathExists(const std::string& path)
{
	const DWORD attr = GetFileAttributes(path.c_str());
	return (attr != INVALID_FILE_ATTRIBUTES);
}

bool FileSystem::IsDirectory(const std::string& path)
{
	const DWORD attr = GetFileAttributes(path.c_str());
	return (attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY));
}

bool FileSystem::IsFile(const std::string& path)
{
	const DWORD attr = GetFileAttributes(path.c_str());
	return (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY));
}

bool FileSystem::CopyFiles(const std::string& source, const std::string& destination, const bool overwrite)
{
	if (!IsPathExists(source))
	{
		return false;
	}

	return CopyFile(source.c_str(),
		destination.c_str(), overwrite);
}

bool FileSystem::MoveFiles(const std::string& source, const std::string& destination)
{
	if (!IsPathExists(source))
	{
		return false;
	}

	return MoveFile(source.c_str(), destination.c_str());
}

std::vector<char> FileSystem::ReadFromFile(const std::string &fileName)
{
	HANDLE file = CreateFile(
			fileName.c_str(),
			GENERIC_READ,
			FILE_SHARE_READ,
			nullptr,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			nullptr
		);

	if (file == INVALID_HANDLE_VALUE) THROW_EXCEPTION_FMT("Failed to open file: {}", fileName);

	LARGE_INTEGER fileSize{};
	if (!GetFileSizeEx(file, &fileSize))
	{
		CloseHandle(file);
		THROW_EXCEPTION_FMT("Failed to get file size: {}", fileName);
	}

	if (fileSize.QuadPart > SIZE_MAX)
	{
		CloseHandle(file);
		THROW_EXCEPTION_MSG("File too large to read into memory.");
	}

	std::vector<char> buffer(static_cast<size_t>(fileSize.QuadPart));

	DWORD bytesRead = 0;
	if (!ReadFile(file, buffer.data(), static_cast<DWORD>(buffer.size()), &bytesRead, nullptr))
	{
		CloseHandle(file);
		THROW_EXCEPTION_FMT("Failed to read file: {}", fileName);
	}

	if (bytesRead != buffer.size())
	{
		CloseHandle(file);
		THROW_EXCEPTION_MSG("Read size mismatch.");
	}

	CloseHandle(file);
	return buffer;
}
