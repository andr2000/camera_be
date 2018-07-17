/* SPDX-License-Identifier: GPL-2.0 */

/*
 * Xen para-virtualized camera backend
 *
 * Copyright (C) 2018 EPAM Systems Inc.
 */
#ifndef SRC_BUFFERSTORAGE_HPP_
#define SRC_BUFFERSTORAGE_HPP_

#include <memory>
#include <unordered_map>

#include <xen/be/Log.hpp>

#include "Buffers.hpp"
#include "Device.hpp"

class BufferStorage
{
public:
	enum class eBufferType {
		BUFFER_TYPE_MMAP,
		BUFFER_TYPE_USRPTR,
		BUFFER_TYPE_DMABUF
	};

	BufferStorage(DevicePtr cameraDev, const int numBuffers,
		      eBufferType type);

	~BufferStorage();

private:
	XenBackend::Log mLog;

	DevicePtr mDev;

	eBufferType mBufType;

	std::vector<BufferPtr> mBuffers;

	void init(int numBuffers);

	void release();
};

typedef std::shared_ptr<BufferStorage> BufferStoragePtr;

#endif /* SRC_BUFFERSTORAGE_HPP_ */
