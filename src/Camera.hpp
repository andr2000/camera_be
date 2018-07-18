/* SPDX-License-Identifier: GPL-2.0 */

/*
 * Xen para-virtualized camera backend
 *
 * Copyright (C) 2018 EPAM Systems Inc.
 */
#ifndef SRC_CAMERA_HPP_
#define SRC_CAMERA_HPP_

#include <xen/be/Log.hpp>

#include "Device.hpp"

#ifdef WITH_DBG_DISPLAY
#include "wayland/Display.hpp"
#endif

class Camera
{
public:
	enum class eAllocMode {
		ALLOC_MMAP,
		ALLOC_USRPTR,
		ALLOC_DMABUF
	};

	Camera(const std::string devName, eAllocMode mode);

	~Camera();

	void start();

	void stop();

private:
	XenBackend::Log mLog;
	std::mutex mLock;
	const std::string mDevName;

	DevicePtr mDev;

	static const int cNumCameraBuffers = 3;

	void init(eAllocMode mode);

	void release();

	void onFrameDoneCallback(int index, int size);

#ifdef WITH_DBG_DISPLAY
	static const int cNumDisplayBuffers = 1;

	DisplayItf::DisplayPtr mDisplay;
	DisplayItf::ConnectorPtr mConnector;
	std::vector<DisplayItf::DisplayBufferPtr> mDisplayBuffer;
	std::vector<DisplayItf::FrameBufferPtr> mFrameBuffer;
	int mCurrentFrameBuffer;

	void startDisplay();
	void stopDisplay();
#endif
};

typedef std::shared_ptr<Camera> CameraPtr;

#endif /* SRC_CAMERA_HPP_ */
