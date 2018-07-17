// SPDX-License-Identifier: GPL-2.0

/*
 * Xen para-virtualized camera backend
 *
 * Based on:
 *   https://linuxtv.org/downloads/v4l-dvb-apis/uapi/v4l/capture.c.html
 *   https://git.ffmpeg.org/ffmpeg.git
 *
 * Copyright (C) 2018 EPAM Systems Inc.
 */
#include <fcntl.h>
#include <unistd.h>

#include <sys/ioctl.h>
#include <sys/stat.h>

#include "Device.hpp"

#include <xen/be/Exception.hpp>

using XenBackend::Exception;

Device::Device(const std::string devName):
	mLog("Device"),
	mDevName(devName),
	mFd(-1),
	mStreamStarted(false)
{
	LOG(mLog, DEBUG) << "Initializing camera device " << devName;

	openDevice();

	getSupportedFormats();
	printSupportedFormats();
}

Device::~Device()
{
	LOG(mLog, DEBUG) << "Deleting camera device " << mDevName;

	stopStream();
	closeDevice();
}

bool Device::isOpen()
{
	return mFd >= 0;
}

void Device::openDevice()
{
	struct stat st;

	if (isOpen())
		return;

	if (stat(mDevName.c_str(), &st) < 0)
		throw Exception("Cannot stat " + mDevName + " video device: " +
				strerror(errno), errno);

	if (!S_ISCHR(st.st_mode))
		throw Exception(mDevName + " is not a character device", EINVAL);

	int fd = open(mDevName.c_str(), O_RDWR /* required */ | O_NONBLOCK, 0);

	if (fd < 0)
		throw Exception("Cannot open " + mDevName + " video device: " +
				strerror(errno), errno);

	mFd = fd;
}

void Device::closeDevice()
{
	if (isOpen())
		close(mFd);

	mFd = -1;
}

int Device::xioctl(int request, void *arg)
{
	int ret;

	if (!isOpen()) {
		errno = EINVAL;
		return -1;
	}

	do {
		ret = ioctl(mFd, request, arg);
	} while (ret == -1 && errno == EINTR);

	return ret;
}

v4l2_format Device::getFormat()
{
	v4l2_format fmt = {0};

	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (xioctl(VIDIOC_G_FMT, &fmt) < 0)
		throw Exception("Failed to call [VIDIOC_G_FMT] for device " +
				mDevName, errno);
	return fmt;
}

void Device::setFormat(v4l2_format fmt)
{
	if (xioctl(VIDIOC_S_FMT, &fmt) < 0)
		throw Exception("Failed to call [VIDIOC_S_FMT] for device " +
				mDevName, errno);
}

int Device::getFrameSize(int index, uint32_t pixelFormat,
			 v4l2_frmsizeenum &size)
{
	memset(&size, 0, sizeof(size));

	size.index = index;
	size.pixel_format = pixelFormat;

	return xioctl(VIDIOC_ENUM_FRAMESIZES, &size);
}

int Device::getFrameInterval(int index, uint32_t pixelFormat,
			     uint32_t width, uint32_t height,
			     v4l2_frmivalenum &interval)
{
	memset(&interval, 0, sizeof(interval));

	interval.index = index;
	interval.pixel_format = pixelFormat;
	interval.width = width;
	interval.height = height;

	return xioctl(VIDIOC_ENUM_FRAMEINTERVALS, &interval);
}

void Device::getSupportedFormats()
{
	v4l2_fmtdesc fmt = {0};

	mFormats.clear();

	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	/* TODO:
	 * 1. Multi-planar formats are not supported yet.
	 * 2. Continuous/step-wise sizes/intervals are not supported.
	 */
	while (xioctl(VIDIOC_ENUM_FMT, &fmt) >= 0) {
		Format format = {
			.pixelFormat = fmt.pixelformat,
			.description = std::string(reinterpret_cast<char *>(fmt.description)),
		};

		v4l2_frmsizeenum size;
		int index = 0;

		while (getFrameSize(index++, fmt.pixelformat, size) >= 0) {
			FormatSize formatSize;

			if (size.type == V4L2_FRMSIZE_TYPE_DISCRETE) {
				v4l2_frmivalenum interval;
				int index = 0;

				formatSize.width = size.discrete.width;
				formatSize.height = size.discrete.height;

				while (getFrameInterval(index++, fmt.pixelformat,
							size.discrete.width,
							size.discrete.height,
							interval) >= 0)
					formatSize.fps.push_back(interval.discrete);
			} else {
				throw Exception("Step-wise/continuous intervals are not supported for device " +
						mDevName, errno);
			}

			format.size.push_back(formatSize);
		}

		fmt.index++;

		mFormats.push_back(format);
	}
}

void Device::printSupportedFormats()
{
	int index = 0;

	for (auto const& format: mFormats) {
		LOG(mLog, DEBUG) << "Format #" << index;
		LOG(mLog, DEBUG) << "\tPixel format: 0x" << std::hex <<
			std::setfill('0') << std::setw(8) << format.pixelFormat;
		LOG(mLog, DEBUG) << "\tDescription: " << format.description;

		std::ostringstream out;
		int index = 0;

		for (auto const& size: format.size) {
			out << "\t\tSize #" << index;
			out << ", resolution: " << size.width << "x" <<
				size.height;
			out << ", FPS:";

			for (auto const& fps: size.fps)
				out << " " << toFps(fps) << ";";

			LOG(mLog, DEBUG) << out.str();
			out.str("");
		}

		index++;
	}
}

int Device::requestBuffers(int numBuffers)
{
	v4l2_requestbuffers req;

	memset(&req, 0, sizeof(req));

	req.count = numBuffers;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_MMAP;

	if (xioctl(VIDIOC_REQBUFS, &req) < 0) {
		if (errno == EINVAL)
			throw Exception("Device doesn't support memory mapping" +
					mDevName, errno);
		else
			throw Exception("Failed to call [VIDIOC_REQBUFS] for device " +
					mDevName, errno);
	}

	LOG(mLog, DEBUG) << "Initialized " << req.count << " buffers for device " << mDevName;

	return req.count;
}

void Device::startStream()
{
	std::lock_guard<std::mutex> lock(mLock);

	if (mStreamStarted)
		return;

	memset(&mCurFormat, 0, sizeof(mCurFormat));

	mCurFormat.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	mCurFormat.fmt.pix.width = 1024;
	mCurFormat.fmt.pix.height = 768;
	mCurFormat.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
	mCurFormat.fmt.pix.field = V4L2_FIELD_NONE;

	setFormat(mCurFormat);
	mCurFormat = getFormat();

	requestBuffers(256);

	v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if (xioctl(VIDIOC_STREAMON, &type) < 0)
		LOG(mLog, ERROR) << "Failed to start streaming on device " << mDevName;

	LOG(mLog, DEBUG) << "Started streaming on device " << mDevName;
}

void Device::stopStream()
{
	std::lock_guard<std::mutex> lock(mLock);

	if (!mStreamStarted)
		return;

	v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if (xioctl(VIDIOC_STREAMOFF, &type) < 0)
		LOG(mLog, ERROR) << "Failed to stop streaming for " << mDevName;

	mStreamStarted = false;
}


