/*******************************************************
 * Copyright (c) 2014, ArrayFire
 * All rights reserved.
 *
 * This file is distributed under 3-clause BSD license.
 * The complete license agreement can be obtained at:
 * http://arrayfire.com/licenses/BSD-3-Clause
 ********************************************************/

#include <memory.hpp>
#include <err_cpu.hpp>
#include <types.hpp>
#include <platform.hpp>
#include <queue.hpp>

#ifndef AF_MEM_DEBUG
#define AF_MEM_DEBUG 0
#endif

#ifndef AF_CPU_MEM_DEBUG
#define AF_CPU_MEM_DEBUG 0
#endif

namespace cpu
{
void setMemStepSize(size_t step_bytes)
{
    memoryManager().setMemStepSize(step_bytes);
}

size_t getMemStepSize(void)
{
    return memoryManager().getMemStepSize();
}

size_t getMaxBytes()
{
    return memoryManager().getMaxBytes();
}

unsigned getMaxBuffers()
{
    return memoryManager().getMaxBuffers();
}

void garbageCollect()
{
    memoryManager().garbageCollect();
}

void printMemInfo(const char *msg, const int device)
{
    memoryManager().printInfo(msg, device);
}

template<typename T>
T* memAlloc(const size_t &elements)
{
    T *ptr = nullptr;

    try {
        ptr = (T *)memoryManager().alloc(elements * sizeof(T), false);
    } catch(...) {
        getQueue().sync();
        ptr = (T *)memoryManager().alloc(elements * sizeof(T), false);
    }
    return ptr;
}

void* memAllocUser(const size_t &bytes)
{
    void *ptr = nullptr;

    try {
        ptr = memoryManager().alloc(bytes, true);
    } catch(...) {
        getQueue().sync();
        ptr = memoryManager().alloc(bytes, true);
    }
    return ptr;
}

template<typename T>
void memFree(T *ptr)
{
    return memoryManager().unlock((void *)ptr, false);
}

void memFreeUser(void *ptr)
{
    memoryManager().unlock((void *)ptr, true);
}

void memLock(const void *ptr)
{
    memoryManager().userLock((void *)ptr);
}

bool isLocked(const void *ptr)
{
    return memoryManager().isUserLocked((void *)ptr);
}

void memUnlock(const void *ptr)
{
    memoryManager().userUnlock((void *)ptr);
}

void deviceMemoryInfo(size_t *alloc_bytes, size_t *alloc_buffers,
                      size_t *lock_bytes,  size_t *lock_buffers)
{
    getQueue().sync();
    memoryManager().bufferInfo(alloc_bytes, alloc_buffers,
                                  lock_bytes,  lock_buffers);
}

template<typename T>
T* pinnedAlloc(const size_t &elements)
{
    return (T *)memoryManager().alloc(elements * sizeof(T), false);
}

template<typename T>
void pinnedFree(T* ptr)
{
    return memoryManager().unlock((void *)ptr, false);
}

bool checkMemoryLimit()
{
    return memoryManager().checkMemoryLimit();
}

#define INSTANTIATE(T)                                      \
    template T* memAlloc(const size_t &elements);           \
    template void memFree(T* ptr);                          \
    template T* pinnedAlloc(const size_t &elements);        \
    template void pinnedFree(T* ptr);                       \

INSTANTIATE(float)
INSTANTIATE(cfloat)
INSTANTIATE(double)
INSTANTIATE(cdouble)
INSTANTIATE(int)
INSTANTIATE(uint)
INSTANTIATE(char)
INSTANTIATE(uchar)
INSTANTIATE(intl)
INSTANTIATE(uintl)
INSTANTIATE(ushort)
INSTANTIATE(short )

MemoryManager::MemoryManager()
    : common::MemoryManager<cpu::MemoryManager>(getDeviceCount(), common::MAX_BUFFERS,
                                                AF_MEM_DEBUG || AF_CPU_MEM_DEBUG)
{
    this->setMaxMemorySize();
}

MemoryManager::~MemoryManager()
{
    common::lock_guard_t lock(this->memory_mutex);
    for (int n = 0; n < cpu::getDeviceCount(); n++) {
        try {
            cpu::setDevice(n);
            garbageCollect();
        } catch(AfError err) {
            continue; // Do not throw any errors while shutting down
        }
    }
}

int MemoryManager::getActiveDeviceId()
{
    return cpu::getActiveDeviceId();
}

size_t MemoryManager::getMaxMemorySize(int id)
{
    return cpu::getDeviceMemorySize(id);
}

void *MemoryManager::nativeAlloc(const size_t bytes)
{
    void *ptr = malloc(bytes);
    if (!ptr) AF_ERROR("Unable to allocate memory", AF_ERR_NO_MEM);
    return ptr;
}

void MemoryManager::nativeFree(void *ptr)
{
    return free((void *)ptr);
}
}
