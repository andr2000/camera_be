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
#include <signal.h>
#include <unistd.h>

#include <sys/ioctl.h>
#include <sys/stat.h>

#include "Device.hpp"

#include <xen/be/Exception.hpp>

using XenBackend::Exception;
using XenBackend::PollFd;

Device::Device(const std::string devName):
	mLog("Device"),
	mDevName(devName),
	mFd(-1),
	mStreamStarted(false),
	mCurMemoryType(0),
	mFrameDoneCallback(nullptr)
{
	LOG(mLog, DEBUG) << "Initializing camera device " << devName;

	openDevice();

	getSupportedFormats();
	printSupportedFormats();

	mPollFd.reset(new PollFd(mFd, POLLIN));
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

	fmt.type = cV4L2BufType;
	if (xioctl(VIDIOC_G_FMT, &fmt) < 0)
		throw Exception("Failed to call [VIDIOC_G_FMT] for device " +
				mDevName, errno);
	return fmt;
}

void Device::setFormat(uint32_t width, uint32_t height, uint32_t pixelFormat)
{
	v4l2_format fmt;

	memset(&fmt, 0, sizeof(fmt));

	fmt.type = cV4L2BufType;
	fmt.fmt.pix.width = width;
	fmt.fmt.pix.height = height;
	fmt.fmt.pix.pixelformat = pixelFormat;
	fmt.fmt.pix.field = V4L2_FIELD_NONE;

	LOG(mLog, DEBUG) << "Set format to " << width << "x" << height;

	if (xioctl(VIDIOC_S_FMT, &fmt) < 0)
		throw Exception("Failed to call [VIDIOC_S_FMT] for device " +
				mDevName, errno);

	mCurFormat = getFormat();

	if ((width != mCurFormat.fmt.pix.width) ||
	    (height != mCurFormat.fmt.pix.height))
		LOG(mLog, ERROR) << "Actual format set to " <<
			mCurFormat.fmt.pix.width << "x" <<
			mCurFormat.fmt.pix.height;
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

	fmt.type = cV4L2BufType;

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
		int szIndex = 0;

		for (auto const& size: format.size) {
			out << "\t\tSize #" << szIndex++;
			out << ", resolution: " << size.width << "x" <<
				size.height;
			out << ", FPS:";

			for (auto const& fps: size.fps)
				out << " " << toFps(fps) << " (" <<
					fps.numerator << "/" <<
					fps.denominator << ");";

			LOG(mLog, DEBUG) << out.str();
			out.str("");
		}

		index++;
	}
}

int Device::requestBuffers(int numBuffers, uint32_t memory)
{
	v4l2_requestbuffers req;

	mCurMemoryType = memory;

	memset(&req, 0, sizeof(req));

	req.count = numBuffers;
	req.type = cV4L2BufType;
	req.memory = memory;

	if (xioctl(VIDIOC_REQBUFS, &req) < 0)
		throw Exception("Failed to call [VIDIOC_REQBUFS] for device " +
				mDevName, errno);

	LOG(mLog, DEBUG) << "Initialized " << req.count << " buffers for device " << mDevName;

	return req.count;
}

v4l2_buffer Device::queryBuffer(int index)
{
	v4l2_buffer buf;

	memset(&buf, 0, sizeof(buf));

	buf.type = cV4L2BufType;
	buf.memory = mCurMemoryType;
	buf.index = index;

	if (xioctl(VIDIOC_QUERYBUF, &buf) < 0)
		throw Exception("Failed to call [VIDIOC_QUERYBUF] for device " +
				mDevName, errno);

	return buf;
}

void Device::queueBuffer(int index)
{
	v4l2_buffer buf;

	memset(&buf, 0, sizeof(buf));

	buf.type = cV4L2BufType;
	buf.memory = mCurMemoryType;
	buf.index = index;

	if (xioctl(VIDIOC_QBUF, &buf) < 0)
		throw Exception("Failed to call [VIDIOC_QBUF] for device " +
				mDevName, errno);
}

v4l2_buffer Device::dequeueBuffer()
{
	v4l2_buffer buf;

	memset(&buf, 0, sizeof(buf));

	buf.type = cV4L2BufType;
	buf.memory = mCurMemoryType;

	if (xioctl(VIDIOC_DQBUF, &buf) < 0)
		throw Exception("Failed to call [VIDIOC_DQBUF] for device " +
				mDevName, errno);

	return buf;
}

int Device::exportBuffer(int index)
{
	v4l2_exportbuffer expbuf = {
		.type = cV4L2BufType,
		.index = static_cast<uint32_t>(index)
	};

	if (xioctl(VIDIOC_EXPBUF, &expbuf))
		throw Exception("Failed to call [VIDIOC_EXPBUF] for device " +
				mDevName, errno);

	return expbuf.fd;
}

void Device::startStream(FrameDoneCallback clb)
{
	std::lock_guard<std::mutex> lock(mLock);

	if (mStreamStarted)
		return;

	mFrameDoneCallback = clb;

	mThread = std::thread(&Device::eventThread, this);

	v4l2_buf_type type = cV4L2BufType;

	if (xioctl(VIDIOC_STREAMON, &type) < 0)
		LOG(mLog, ERROR) << "Failed to start streaming on device " << mDevName;

	mStreamStarted = true;

	LOG(mLog, DEBUG) << "Started streaming on device " << mDevName;
}

void Device::stopStream()
{
	std::lock_guard<std::mutex> lock(mLock);

	if (!mStreamStarted)
		return;

	if (mPollFd)
		mPollFd->stop();

	if (mThread.joinable())
		mThread.join();

	v4l2_buf_type type = cV4L2BufType;

	if (xioctl(VIDIOC_STREAMOFF, &type) < 0)
		LOG(mLog, ERROR) << "Failed to stop streaming for " << mDevName;

	mStreamStarted = false;

	LOG(mLog, DEBUG) << "Stoped streaming on device " << mDevName;
}

void Device::eventThread()
{
	try {
		while (mPollFd->poll()) {
			v4l2_buffer buf = dequeueBuffer();
			if (mFrameDoneCallback)
				mFrameDoneCallback(buf.index, buf.bytesused);
			queueBuffer(buf.index);
		}
	} catch(const std::exception& e) {
		LOG(mLog, ERROR) << e.what();

		kill(getpid(), SIGTERM);
	}
}

