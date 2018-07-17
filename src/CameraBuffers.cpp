// SPDX-License-Identifier: GPL-2.0

/*
 * Xen para-virtualized camera backend
 *
 * Copyright (C) 2018 EPAM Systems Inc.
 */

#include "CameraBuffers.hpp"

#include <xen/be/Exception.hpp>

using XenBackend::Exception;

CameraBufferMmap::CameraBufferMmap(CameraDevicePtr cameraDev):
	CameraBufferItf(cameraDev)
{
	LOG(mLog, DEBUG) << "Allocating MMAP camera buffer for " <<
		mCameraDev->getName();
}

CameraBufferMmap::~CameraBufferMmap()
{
	LOG(mLog, DEBUG) << "Deleting MMAP camera buffer for " <<
		mCameraDev->getName();
}

CameraBufferUsrPtr::CameraBufferUsrPtr(CameraDevicePtr cameraDev):
	CameraBufferItf(cameraDev)
{
	LOG(mLog, DEBUG) << "Allocating USRPTR camera buffer for " <<
		mCameraDev->getName();
}

CameraBufferUsrPtr::~CameraBufferUsrPtr()
{
	LOG(mLog, DEBUG) << "Deleting USRPTR camera buffer for " <<
		mCameraDev->getName();
}

CameraBufferDmaBuf::CameraBufferDmaBuf(CameraDevicePtr cameraDev):
	CameraBufferItf(cameraDev)
{
	LOG(mLog, DEBUG) << "Allocating DMABUF camera buffer for " <<
		mCameraDev->getName();
}

CameraBufferDmaBuf::~CameraBufferDmaBuf()
{
	LOG(mLog, DEBUG) << "Deleting DMABUF camera buffer for " <<
		mCameraDev->getName();
}

