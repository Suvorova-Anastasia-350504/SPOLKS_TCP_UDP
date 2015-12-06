#pragma once
#include "Base.h"


struct SharedMemoryDescriptor
{
	HANDLE handle;
	std::string name;
	void* memory;
	size_t size;
};

class SharedMemoryManager
{
public:
	static SharedMemoryDescriptor CreateSharedMemory(size_t size, const std::string& name);
	static SharedMemoryDescriptor OpenSharedMemory(size_t size, const std::string& name);
	static void RemoveSharedMemory(SharedMemoryDescriptor& desc);
};

