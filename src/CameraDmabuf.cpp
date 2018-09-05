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

#include "CameraDmabuf.hpp"

#include <xen/be/Exception.hpp>

using XenBackend::Exception;

CameraDmabuf::CameraDmabuf(const std::string devName):
    CameraMmap(devName)
{
    LOG(mLog, DEBUG) << "Using DMABUF extensions";
}

CameraDmabuf::~CameraDmabuf()
{
}

void CameraDmabuf::allocStream(int numBuffers, uint32_t width,
                               uint32_t height, uint32_t pixelFormat)
{
    std::lock_guard<std::mutex> lock(mLock);

    CameraMmap::allocStreamUnlocked(numBuffers, width, height, pixelFormat);

    mBufferFds.clear();
    for  (size_t i = 0; i < mBuffers.size(); i++)
        mBufferFds.push_back(exportBuffer(i));
}

void CameraDmabuf::releaseStream()
{
    std::lock_guard<std::mutex> lock(mLock);

    CameraMmap::releaseStreamUnlocked();

    for (auto const& fd: mBufferFds)
        close(fd);

    mBufferFds.clear();
}


