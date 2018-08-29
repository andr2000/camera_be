// SPDX-License-Identifier: GPL-2.0

/*
 * Xen para-virtualized camera backend
 *
 * Copyright (C) 2018 EPAM Systems Inc.
 */

#include "Camera.hpp"
#include "DeviceMmap.hpp"
#include "DeviceDmabuf.hpp"

#include <xen/be/Exception.hpp>

using namespace std::placeholders;

using XenBackend::Exception;

Camera::Camera(const std::string devName, eAllocMode mode):
	mLog("Camera"),
	mDevName(devName),
	mDev(nullptr),
	mDisplay(nullptr)
{
	try {
		init(mode);
	} catch (Exception &e) {
		release();
		throw Exception("Failed to initialize camera " + mDevName,
				ENOTTY);
	}
}

Camera::~Camera()
{
	release();
}

void Camera::init(eAllocMode mode)
{
	LOG(mLog, DEBUG) << "Initializing camera " << mDevName;

	switch (mode) {
	case eAllocMode::ALLOC_MMAP:
		mDev = DevicePtr(new DeviceMmap(mDevName));
		break;

	case eAllocMode::ALLOC_DMABUF:
		mDev = DevicePtr(new DeviceDmabuf(mDevName));
		break;

	default:
		throw Exception("Unknown camera alloc mode", EINVAL);
	}

#ifdef WITH_DBG_DISPLAY
	mDisplay = Wayland::DisplayPtr(new Wayland::Display());

	mDisplay->start();

	mConnector = mDisplay->createConnector("Camera debug display");
#endif
}

void Camera::release()
{
	LOG(mLog, DEBUG) << "Releasing camera " << mDevName;

	if (!mDev)
		return;

	mDev->stopStream();
	mDev->releaseStream();

#ifdef WITH_DBG_DISPLAY
	stopDisplay();
#endif
}

void Camera::start()
{
	mDev->allocStream(cNumCameraBuffers, 640, 480, V4L2_PIX_FMT_YUYV);
#ifdef WITH_DBG_DISPLAY
	startDisplay();
#endif
	mDev->startStream(bind(&Camera::onFrameDoneCallback, this, _1, _2));
}

void Camera::stop()
{
	release();
}

void Camera::onFrameDoneCallback(int index, int size)
{
#ifdef WITH_DBG_DISPLAY
	auto data = mDev->getBufferData(index);

	memcpy(mDisplayBuffer[mCurrentFrameBuffer]->getBuffer(), data, size);

	mConnector->pageFlip(mFrameBuffer[mCurrentFrameBuffer], nullptr);
	mDisplay->flush();
#endif
}

#ifdef WITH_DBG_DISPLAY
void Camera::startDisplay()
{
	v4l2_format fmt = mDev->getCurrentFormat();
	uint32_t width = fmt.fmt.pix.width;
	uint32_t height = fmt.fmt.pix.height;

	mDisplayBuffer.clear();
	mFrameBuffer.clear();

	for (size_t i = 0; i < cNumDisplayBuffers; i++) {
		auto db = mDisplay->createDisplayBuffer(width, height, 16);

		mDisplayBuffer.push_back(db);
		mFrameBuffer.push_back(mDisplay->createFrameBuffer(db,
			width, height, WL_SHM_FORMAT_YUYV));
	}

	mCurrentFrameBuffer = 0;
	mConnector->init(width, height, mFrameBuffer[mCurrentFrameBuffer]);

	mDisplay->flush();
}

void Camera::stopDisplay()
{
	if (!mDisplay)
		return;

	mDisplay->stop();

	mFrameBuffer.clear();
	mConnector->release();
}
#endif

