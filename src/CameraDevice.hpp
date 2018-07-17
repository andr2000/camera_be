/* SPDX-License-Identifier: GPL-2.0 */

/*
 * Xen para-virtualized camera backend
 *
 * Copyright (C) 2018 EPAM Systems Inc.
 */
#ifndef SRC_CAMERADEVICE_HPP_
#define SRC_CAMERADEVICE_HPP_

#include <list>
#include <memory>
#include <mutex>
#include <string>

#include <linux/videodev2.h>

#include <xen/be/Log.hpp>

class CameraDevice
{
public:
	CameraDevice(const std::string devName);

	~CameraDevice();

	const std::string getName() const {
		return mDevName;
	}

	v4l2_format getCurrentFormat() const {
		return mCurFormat;
	}

	void startStream();
	void stopStream();

	int requestBuffers(int numBuffers);

private:
	struct FormatSize {
		int width;
		int height;
		std::vector<v4l2_fract> fps;
	};

	struct Format {
		uint32_t pixelFormat;
		std::string description;

		std::vector<FormatSize> size;
	};

	XenBackend::Log mLog;

	std::mutex mLock;

	const std::string mDevName;

	int mFd;

	bool mStreamStarted;

	std::vector<std::string> mVideoNodes;

	v4l2_format mCurFormat;

	std::vector<Format> mFormats;

	int xioctl(int request, void *arg);

	bool isOpen();
	void openDevice();
	void closeDevice();

	void getSupportedFormats();
	void printSupportedFormats();
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

};

typedef std::shared_ptr<CameraDevice> CameraDevicePtr;

#endif /* SRC_CAMERADEVICE_HPP_ */
