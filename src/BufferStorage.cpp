// SPDX-License-Identifier: GPL-2.0

/*
 * Xen para-virtualized camera backend
 *
 * Copyright (C) 2018 EPAM Systems Inc.
 */

#include "BufferStorage.hpp"

#include <xen/be/Exception.hpp>

using XenBackend::Exception;

BufferStorage::BufferStorage(DevicePtr cameraDev, const int numBuffers,
			     eBufferType type):
	mLog("BufferStorage"),
	mDev(cameraDev),
	mBufType(type)
{
	LOG(mLog, DEBUG) << "Initializing buffer storage for " <<
		mDev->getName() << " with " << numBuffers <<
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
		mDev->getName();

	release();
}

void BufferStorage::init(int numBuffers)
{
	int numAllocated = mDev->requestBuffers(numBuffers);

	if (numAllocated != numBuffers)
		LOG(mLog, WARNING) << "Allocated " << numAllocated <<
			", expected " << numBuffers;

	for (int i = 0; i < numAllocated; i++) {
		BufferPtr buffer;

		switch (mBufType) {
		case eBufferType::BUFFER_TYPE_MMAP:
			buffer = BufferPtr(new BufferMmap(mDev));
			break;

		case eBufferType::BUFFER_TYPE_USRPTR:
			buffer = BufferPtr(new BufferUsrPtr(mDev));
			break;

		case eBufferType::BUFFER_TYPE_DMABUF:
			buffer = BufferPtr(new BufferDmaBuf(mDev));
			break;
		}

		mBuffers.push_back(buffer);
	}
}

void BufferStorage::release()
{
}

