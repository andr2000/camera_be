// SPDX-License-Identifier: GPL-2.0

/*
 * Xen para-virtualized camera backend
 *
 * Based on:
 *   https://linuxtv.org/downloads/v4l-dvb-apis/uapi/v4l/capture.c.html
 *   https://git.ffmpeg.org/ffmpeg.git
 *
 * Copyright (C) 2018 EPAM Systems Inc.
 */

#include <sys/mman.h>

#include "CameraMmap.hpp"

#include <xen/be/Exception.hpp>

using XenBackend::Exception;

CameraMmap::CameraMmap(const std::string devName):
    Camera(devName)
{
    LOG(mLog, DEBUG) << "Using V4L2_MEMORY_MMAP for memory allocations";
}

CameraMmap::~CameraMmap()
{
}

void CameraMmap::allocStreamUnlocked(int numBuffers, uint32_t width,
                                     uint32_t height, uint32_t pixelFormat)
{
    setFormat(width, height, pixelFormat);

    int numAllocated = requestBuffers(numBuffers, V4L2_MEMORY_MMAP);

    if (numAllocated != numBuffers)
        LOG(mLog, WARNING) << "Allocated " << numAllocated <<
            ", expected " << numBuffers;

    for (int i = 0; i < numAllocated; i++) {
        v4l2_buffer buf = queryBuffer(i);

        void *start = mmap(nullptr /* start anywhere */,
                           buf.length,
                           PROT_READ | PROT_WRITE /* required */,
                           MAP_SHARED /* recommended */,
                           mFd, buf.m.offset);

        if (start == MAP_FAILED)
            throw Exception("Failed to mmap buffer for device " +
                            mDevPath, errno);

        queueBuffer(i);

        mBuffers.push_back({
                           .size = static_cast<size_t>(buf.length),
                           .data = start
                           }
                          );
    }
}

void CameraMmap::releaseStreamUnlocked()
{
    for (auto const& buffer: mBuffers)
        munmap(buffer.data, buffer.size);

    mBuffers.clear();
}

void CameraMmap::allocStream(int numBuffers, uint32_t width,
                             uint32_t height, uint32_t pixelFormat)
{
    std::lock_guard<std::mutex> lock(mLock);

    allocStreamUnlocked(numBuffers, width, height, pixelFormat);
}

void CameraMmap::releaseStream()
{
    std::lock_guard<std::mutex> lock(mLock);

    releaseStreamUnlocked();
}

