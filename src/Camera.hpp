/* SPDX-License-Identifier: GPL-2.0 */

/*
 * Xen para-virtualized camera backend
 *
 * Copyright (C) 2018 EPAM Systems Inc.
 */
#ifndef SRC_CAMERA_HPP_
#define SRC_CAMERA_HPP_

#include <list>
#include <memory>
#include <mutex>
#include <string>

#include <linux/videodev2.h>

#include <xen/be/Log.hpp>

#ifdef WITH_DBG_DISPLAY
#include "wayland/Display.hpp"
#endif

class Camera
{
public:
	Camera(const std::string devName);

	~Camera();

	void startStream();

	void stopStream();

private:
	XenBackend::Log mLog;

	std::mutex mLock;

	const std::string mDevName;

	int mFd;

	bool mStreamStarted;

	std::vector<std::string> mVideoNodes;

	/* Current format in use */
	v4l2_format mCurFormat;

#ifdef WITH_DBG_DISPLAY
	static const int cNumDisplayBuffers = 2;

	DisplayItf::DisplayPtr mDisplay;
	DisplayItf::ConnectorPtr mConnector;
	std::vector<DisplayItf::FrameBufferPtr> mFrameBuffer;
	int mCurrentFrameBuffer;

	void startDisplay();
	void stopDisplay();
#endif

	int xioctl(int request, void *arg);

	bool isOpen();

	void openDevice();

	void closeDevice();

	void getSupportedFormats();

	v4l2_format getFormat();

	void setFormat(v4l2_format fmt);

	int getFrameSize(int index, uint32_t pixelFormat,
			 v4l2_frmsizeenum &size);

	int getFrameInterval(int index, uint32_t pixelFormat,
			     uint32_t width, uint32_t height,
			     v4l2_frmivalenum &interval);

	static float toFps(const v4l2_fract &fract) {
		return static_cast<float>(fract.denominator) / fract.numerator;
	}

	void requestBuffers();
};

typedef std::shared_ptr<Camera> CameraPtr;

#endif /* SRC_CAMERA_HPP_ */
