/* SPDX-License-Identifier: GPL-2.0 */

/*
 * Xen para-virtualized camera backend
 *
 * Copyright (C) 2018 EPAM Systems Inc.
 */
#ifndef SRC_CAMERA_HPP_
#define SRC_CAMERA_HPP_

#include <xen/be/Log.hpp>

#include "BufferStorage.hpp"
#include "Device.hpp"

#ifdef WITH_DBG_DISPLAY
#include "wayland/Display.hpp"
#endif

class Camera
{
public:
	Camera(const std::string devName);

	~Camera();

	void start();

private:
	XenBackend::Log mLog;
	std::mutex mLock;
	const std::string mDevName;

	DevicePtr mDev;

	static const int cNumCameraBuffers = 3;
	BufferStoragePtr mBufStore;

	void init();

	void release();

#ifdef WITH_DBG_DISPLAY
	static const int cNumDisplayBuffers = 2;

	DisplayItf::DisplayPtr mDisplay;
	DisplayItf::ConnectorPtr mConnector;
	std::vector<DisplayItf::FrameBufferPtr> mFrameBuffer;
	int mCurrentFrameBuffer;

	void startDisplay();
	void stopDisplay();
#endif
};

typedef std::shared_ptr<Camera> CameraPtr;

#endif /* SRC_CAMERA_HPP_ */
