/* SPDX-License-Identifier: GPL-2.0 */

/*
 * Xen para-virtualized camera backend
 *
 * Copyright (C) 2018 EPAM Systems Inc.
 */
#ifndef SRC_BUFFERS_HPP_
#define SRC_BUFFERS_HPP_

#include <memory>

#include <xen/be/Log.hpp>

#include "Device.hpp"

class BufferItf
{
public:
	BufferItf(DevicePtr cameraDev):
		mLog("Buffers"),
		mDev(cameraDev) {}

	virtual ~BufferItf() {}

protected:
	XenBackend::Log mLog;

	DevicePtr mDev;
};

class BufferMmap : public BufferItf
{
public:
	BufferMmap(DevicePtr cameraDev);

	~BufferMmap();
};

class BufferUsrPtr : public BufferItf
{
public:
	BufferUsrPtr(DevicePtr cameraDev);

	~BufferUsrPtr();
};

class BufferDmaBuf : public BufferItf
{
public:
	BufferDmaBuf(DevicePtr cameraDev);

	~BufferDmaBuf();
};

typedef std::shared_ptr<BufferItf> BufferPtr;

#endif /* SRC_BUFFERS_HPP_ */
