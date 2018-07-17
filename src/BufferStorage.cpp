// SPDX-License-Identifier: GPL-2.0

/*
 * Xen para-virtualized camera backend
 *
 * Copyright (C) 2018 EPAM Systems Inc.
 */

#include "BufferStorage.hpp"

#include <xen/be/Exception.hpp>

using XenBackend::Exception;

BufferStorage::BufferStorage(CameraDevicePtr cameraDev, const int numBuffers,
			     eBufferType type):
	mLog("BufferStorage"),
	mCameraDev(cameraDev),
	mBufType(type)
{
	LOG(mLog, DEBUG) << "Initializing buffer storage for " <<
		mCameraDev->getName() << " with " << numBuffers <<
		" buffers requested";

	try {
		init(numBuffers);
	} catch (Exception &e) {
		release();
	}
}

BufferStorage::~BufferStorage()
{
	LOG(mLog, DEBUG) << "Deleting buffer storage for " <<
		mCameraDev->getName();

	release();
}

void BufferStorage::init(int numBuffers)
{
	int numAllocated = mCameraDev->requestBuffers(numBuffers);

	if (numAllocated != numBuffers)
		LOG(mLog, WARNING) << "Allocated " << numAllocated <<
			", expected " << numBuffers;

	for (int i = 0; i < numAllocated; i++) {
		CameraBufferPtr buffer;

		switch (mBufType) {
		case eBufferType::BUFFER_TYPE_MMAP:
			buffer = CameraBufferPtr(new CameraBufferMmap(mCameraDev));
			break;

		case eBufferType::BUFFER_TYPE_USRPTR:
			buffer = CameraBufferPtr(new CameraBufferUsrPtr(mCameraDev));
			break;

		case eBufferType::BUFFER_TYPE_DMABUF:
			buffer = CameraBufferPtr(new CameraBufferDmaBuf(mCameraDev));
			break;
		}
		mCameraBuffers.push_back(buffer);
	}
}

void BufferStorage::release()
{
}

