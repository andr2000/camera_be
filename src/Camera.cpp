// SPDX-License-Identifier: GPL-2.0

/*
 * Xen para-virtualized camera backend
 *
 * Copyright (C) 2018 EPAM Systems Inc.
 */

#include "Camera.hpp"

#include <xen/be/Exception.hpp>

using XenBackend::Exception;

Camera::Camera(const std::string devName):
	mLog("Camera"),
	mDevName(devName)
{
	LOG(mLog, DEBUG) << "Initializing camera " << devName;

	try {
		init();
	} catch (Exception &e) {
	}
}

Camera::~Camera()
{
	LOG(mLog, DEBUG) << "Deleting camera " << mDevName;

	release();
}

void Camera::init()
{

	mDev = CameraDevicePtr(new CameraDevice(mDevName));

	mBufStore = BufferStoragePtr(new BufferStorage(mDev, cNumCameraBuffers,
						       BufferStorage::eBufferType::BUFFER_TYPE_MMAP));

#ifdef WITH_DBG_DISPLAY
	mDisplay = Wayland::DisplayPtr(new Wayland::Display());

	mDisplay->start();

	mConnector = mDisplay->createConnector("Camera debug display");
#endif
}

void Camera::release()
{
#ifdef WITH_DBG_DISPLAY
	mDisplay->stop();
#endif
}

void Camera::start()
{
}

#ifdef WITH_DBG_DISPLAY
void Camera::startDisplay()
{
	v4l2_format fmt = mDev->getCurrentFormat();
	uint32_t width = fmt.fmt.pix.width;
	uint32_t height = fmt.fmt.pix.height;

	mFrameBuffer.clear();

	for (size_t i = 0; i < cNumDisplayBuffers; i++) {
		auto db = mDisplay->createDisplayBuffer(width, height, 32);

		mFrameBuffer.push_back(mDisplay->createFrameBuffer(db,
								   width,
								   height,
								   1));
	}

	mConnector->init(width, height, mFrameBuffer[mCurrentFrameBuffer]);

	mDisplay->flush();
}

void Camera::stopDisplay()
{
	mFrameBuffer.clear();
	mConnector->release();
}
#endif

