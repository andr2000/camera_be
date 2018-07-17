// SPDX-License-Identifier: GPL-2.0

/*
 * Xen para-virtualized camera backend
 *
 * Copyright (C) 2018 EPAM Systems Inc.
 */

#include "Buffers.hpp"

#include <xen/be/Exception.hpp>

using XenBackend::Exception;

BufferMmap::BufferMmap(DevicePtr cameraDev):
	BufferItf(cameraDev)
{
	LOG(mLog, DEBUG) << "Allocating MMAP camera buffer for " <<
		mDev->getName();
}

BufferMmap::~BufferMmap()
{
	LOG(mLog, DEBUG) << "Deleting MMAP camera buffer for " <<
		mDev->getName();
}

BufferUsrPtr::BufferUsrPtr(DevicePtr cameraDev):
	BufferItf(cameraDev)
{
	LOG(mLog, DEBUG) << "Allocating USRPTR camera buffer for " <<
		mDev->getName();
}

BufferUsrPtr::~BufferUsrPtr()
{
	LOG(mLog, DEBUG) << "Deleting USRPTR camera buffer for " <<
		mDev->getName();
}

BufferDmaBuf::BufferDmaBuf(DevicePtr cameraDev):
	BufferItf(cameraDev)
{
	LOG(mLog, DEBUG) << "Allocating DMABUF camera buffer for " <<
		mDev->getName();
}

BufferDmaBuf::~BufferDmaBuf()
{
	LOG(mLog, DEBUG) << "Deleting DMABUF camera buffer for " <<
		mDev->getName();
}

