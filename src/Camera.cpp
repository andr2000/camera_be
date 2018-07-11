// SPDX-License-Identifier: GPL-2.0

/*
 * Xen para-virtualized camera backend
 *
 * Based on: https://linuxtv.org/downloads/v4l-dvb-apis/uapi/v4l/capture.c.html
 *
 * Copyright (C) 2018 EPAM Systems Inc.
 */
#include <fcntl.h>
#include <unistd.h>

#include <sys/ioctl.h>
#include <sys/stat.h>

#include "Camera.hpp"

#include <xen/be/Exception.hpp>

using XenBackend::Exception;

Camera::Camera(const std::string devName):
	mLog("Camera"), mDevName(devName), mFd(-1),
	mStreamStarted(false)
{
	LOG(mLog, DEBUG) << "Initializing camera device " << devName;

	openDevice();
	getSupportedFormats();

#ifdef WITH_DBG_DISPLAY
	mDisplay = Wayland::DisplayPtr(new Wayland::Display());

	mDisplay->start();

	mConnector = mDisplay->createConnector("Camera debug display");
#endif
}

Camera::~Camera()
{
	LOG(mLog, DEBUG) << "Deleting camera device " << mDevName;

#ifdef WITH_DBG_DISPLAY
	mDisplay->stop();
#endif

	stopStream();
	closeDevice();
}

bool Camera::isOpen()
{
	return mFd >= 0;
}

void Camera::openDevice()
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

void Camera::closeDevice()
{
	if (isOpen())
		close(mFd);

	mFd = -1;
}

int Camera::xioctl(int request, void *arg)
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

v4l2_format Camera::getFormat()
{
	v4l2_format fmt = {0};

	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (xioctl(VIDIOC_G_FMT, &fmt) < 0)
		throw Exception("Failed to call [VIDIOC_G_FMT] for device " +
				mDevName, errno);
	return fmt;
}

void Camera::setFormat(v4l2_format fmt)
{
	if (xioctl(VIDIOC_S_FMT, &fmt) < 0)
		throw Exception("Failed to call [VIDIOC_S_FMT] for device " +
				mDevName, errno);
}

int Camera::getFrameSize(int index, uint32_t pixelFormat,
			 v4l2_frmsizeenum &size)
{
	memset(&size, 0, sizeof(size));

	size.index = index;
	size.pixel_format = pixelFormat;

	return xioctl(VIDIOC_ENUM_FRAMESIZES, &size);
}

int Camera::getFrameInterval(int index, uint32_t pixelFormat,
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

void Camera::getSupportedFormats()
{
	v4l2_fmtdesc fmt = {0};

	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	/* TODO:
	 * 1. Multi-planar formats are not supported yet.
	 * 2. Continuous/step-wise sizes/intervals are not supported.
	 */
	while (xioctl(VIDIOC_ENUM_FMT, &fmt) >= 0) {
		LOG(mLog, DEBUG) << "Format #" << fmt.index;
		LOG(mLog, DEBUG) << "\tPixel format: 0x" << std::hex <<
			std::setfill('0') << std::setw(8) << fmt.pixelformat;
		LOG(mLog, DEBUG) << "\tDescription: " << fmt.description;

		std::ostringstream out;
		v4l2_frmsizeenum size;
		int index = 0;

		while (getFrameSize(index++, fmt.pixelformat, size) >= 0) {
			out << "\t\tSize #" << size.index;
			out << ", type: " << (size.type == V4L2_FRMSIZE_TYPE_DISCRETE ?
					     "discrete" : size.type == V4L2_FRMSIZE_TYPE_CONTINUOUS ?
					     "continuous" : "step-wise");

			if (size.type == V4L2_FRMSIZE_TYPE_DISCRETE) {
				v4l2_frmivalenum interval;
				int index = 0;

				out << ", resolution: " <<
					size.discrete.width << "x" <<
					size.discrete.height;


				out << ", FPS:";
				while (getFrameInterval(index++, fmt.pixelformat,
							size.discrete.width,
							size.discrete.height,
							interval) >= 0) {
					out << " " << toFps(interval.discrete) << ";";
				}
			} else {
				throw Exception("Step-wise/continuous intervals are not supported for device " +
						mDevName, errno);
			}

			LOG(mLog, DEBUG) << out.str();
			out.str("");
		}

		fmt.index++;
	}
}

void Camera::requestBuffers(int numBuffers)
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
}

#if 0
void Camera::mapBuffers()
{
    buffers = calloc(req.count, sizeof(*buffers));

    if (!buffers) {
	    fprintf(stderr, "Out of memory\\n");
	    exit(EXIT_FAILURE);
    }

    for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {
	    struct v4l2_buffer buf;

	    CLEAR(buf);

	    buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	    buf.memory      = V4L2_MEMORY_MMAP;
	    buf.index       = n_buffers;

	    if (-1 == xioctl(fd, VIDIOC_QUERYBUF, &buf))
		    errno_exit("VIDIOC_QUERYBUF");

	    buffers[n_buffers].length = buf.length;
	    buffers[n_buffers].start =
		    mmap(NULL /* start anywhere */,
			  buf.length,
			  PROT_READ | PROT_WRITE /* required */,
			  MAP_SHARED /* recommended */,
			  fd, buf.m.offset);

	    if (MAP_FAILED == buffers[n_buffers].start)
		    errno_exit("mmap");
    }
}
#endif

void Camera::startStream()
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

	requestBuffers();

#ifdef WITH_DBG_DISPLAY
	startDisplay();
#endif

	v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if (xioctl(VIDIOC_STREAMON, &type) < 0)
		LOG(mLog, ERROR) << "Failed to start streaming on device " << mDevName;

	LOG(mLog, DEBUG) << "Started streaming on device " << mDevName;
}

void Camera::stopStream()
{
	std::lock_guard<std::mutex> lock(mLock);

	if (!mStreamStarted)
		return;

	v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if (xioctl(VIDIOC_STREAMOFF, &type) < 0)
		LOG(mLog, ERROR) << "Failed to stop streaming for " << mDevName;

#ifdef WITH_DBG_DISPLAY
	stopDisplay();
#endif

	mStreamStarted = false;
}

#ifdef WITH_DBG_DISPLAY
void Camera::startDisplay()
{
	uint32_t width = mCurFormat.fmt.pix.width;
	uint32_t height = mCurFormat.fmt.pix.height;

	mFrameBuffer.clear();
	mCurrentFrameBuffer = 0;

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
