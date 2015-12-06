#include "SharedMemoryManager.h"

SharedMemoryDescriptor SharedMemoryManager::CreateSharedMemory(size_t size, const std::string& name)
{
	SharedMemoryDescriptor result;

	result.name = name;
	result.size = size;
#ifdef _WIN32
	result.handle = CreateFileMapping(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, size, name.c_str());
	result.memory = MapViewOfFile(result.handle, FILE_MAP_ALL_ACCESS, 0, 0, size);
#else
	result.handle = shm_open(name.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRWXU | S_IRGRP | S_IROTH);
	ftruncate(result.handle, size);
	result.memory = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, result.handle, 0);
#endif
	return result;
}

SharedMemoryDescriptor SharedMemoryManager::OpenSharedMemory(size_t size, const std::string& name)
{
	SharedMemoryDescriptor result;

	result.name = name;
	result.size = size;
#ifdef _WIN32
	result.handle = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, name.c_str());
	result.memory = MapViewOfFile(result.handle, FILE_MAP_ALL_ACCESS, 0, 0, size);
#else
	result.handle = shm_open(name.c_str(), O_RDWR, 0);
	result.memory = mmap(nullptr, size, PROT_READ, MAP_SHARED, result.handle, 0);
#endif
	return result;
}

void SharedMemoryManager::RemoveSharedMemory(SharedMemoryDescriptor& desc)
{
#ifdef _WIN32
	UnmapViewOfFile(desc.memory);
	CloseHandle(desc.handle);
#else
	shm_unlink(desc.name.c_str());
	munmap(desc.memory, desc.size);
#endif
}

