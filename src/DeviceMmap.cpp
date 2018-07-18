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

#include "DeviceMmap.hpp"

#include <xen/be/Exception.hpp>

using XenBackend::Exception;

DeviceMmap::DeviceMmap(const std::string devName):
	Device(devName)
{
	LOG(mLog, DEBUG) << "Using V4L2_MEMORY_MMAP for memory allocations";
}

DeviceMmap::~DeviceMmap()
{
}

void DeviceMmap::allocStream(int numBuffers, uint32_t width,
			     uint32_t height, uint32_t pixelFormat)
{
	std::lock_guard<std::mutex> lock(mLock);

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
					mDevName, errno);

		queueBuffer(i);

		mBuffers.push_back({
				.size = static_cast<size_t>(buf.length),
				.data = start
			}
		);
	}
}

void DeviceMmap::releaseStream()
{
	for (auto const& buffer: mBuffers)
		munmap(buffer.data, buffer.size);
}


