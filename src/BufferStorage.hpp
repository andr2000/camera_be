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

#include "CameraBuffers.hpp"
#include "CameraDevice.hpp"

class BufferStorage
{
public:
	enum class eBufferType {
		BUFFER_TYPE_MMAP,
		BUFFER_TYPE_USRPTR,
		BUFFER_TYPE_DMABUF
	};

	BufferStorage(CameraDevicePtr cameraDev, const int numBuffers,
		      eBufferType type);

	~BufferStorage();

private:
	XenBackend::Log mLog;

	CameraDevicePtr mCameraDev;

	eBufferType mBufType;

	std::vector<CameraBufferPtr> mCameraBuffers;

	void init(int numBuffers);

	void release();
};

typedef std::shared_ptr<BufferStorage> BufferStoragePtr;

#endif /* SRC_BUFFERSTORAGE_HPP_ */
