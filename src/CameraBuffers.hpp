/* SPDX-License-Identifier: GPL-2.0 */

/*
 * Xen para-virtualized camera backend
 *
 * Copyright (C) 2018 EPAM Systems Inc.
 */
#ifndef SRC_CAMERABUFFERS_HPP_
#define SRC_CAMERABUFFERS_HPP_

#include <memory>

#include <xen/be/Log.hpp>

#include "CameraDevice.hpp"

class CameraBufferItf
{
public:
	CameraBufferItf(CameraDevicePtr cameraDev):
		mLog("CameraBuffer"),
		mCameraDev(cameraDev) {}

	virtual ~CameraBufferItf() {}

protected:
	XenBackend::Log mLog;

	CameraDevicePtr mCameraDev;
};

class CameraBufferMmap : public CameraBufferItf
{
public:
	CameraBufferMmap(CameraDevicePtr cameraDev);

	~CameraBufferMmap();
};

class CameraBufferUsrPtr : public CameraBufferItf
{
public:
	CameraBufferUsrPtr(CameraDevicePtr cameraDev);

	~CameraBufferUsrPtr();
};

class CameraBufferDmaBuf : public CameraBufferItf
{
public:
	CameraBufferDmaBuf(CameraDevicePtr cameraDev);

	~CameraBufferDmaBuf();
};

typedef std::shared_ptr<CameraBufferItf> CameraBufferPtr;

#endif /* SRC_CAMERABUFFERS_HPP_ */
