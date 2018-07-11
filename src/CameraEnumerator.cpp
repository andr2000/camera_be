// SPDX-License-Identifier: GPL-2.0

/*
 * Xen para-virtualized camera backend
 *
 * Based on: https://linuxtv.org/downloads/v4l-dvb-apis/uapi/v4l/capture.c.html
 *
 * Copyright (C) 2018 EPAM Systems Inc.
 */

#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>

#include <linux/videodev2.h>

#include <sys/ioctl.h>
#include <sys/stat.h>

#include <xen/be/Exception.hpp>

#include "CameraEnumerator.hpp"

CameraEnumerator::CameraEnumerator():
	mLog("CameraEnumerator")
{
	try {
		init();
	} catch(const XenBackend::Exception &e) {
		release();

		throw;
	}
}

int CameraEnumerator::xioctl(int fd, int request, void *arg)
{
	int ret;

	do {
		ret = ioctl(fd, request, arg);
	} while (ret == -1 && errno == EINTR);

	return ret;
}

int CameraEnumerator::enumerateVideoNodes()
{
	DIR *dir = opendir("/dev");

	if (dir == NULL) {
		LOG(mLog, ERROR) << "Failed to open /dev";
		return -1;
	}

	/* Go over all video device nodes. */
	static const std::string cVideoDevBaseName = "video";
	size_t nameLen = cVideoDevBaseName.size();
	dirent *dirEnt;

	mVideoNodes.clear();
	while ((dirEnt = readdir(dir))) {
		if (strncmp(cVideoDevBaseName.c_str(), dirEnt->d_name, nameLen))
			continue;

		mVideoNodes.push_back(std::string("/dev/") + dirEnt->d_name);
	}

	closedir(dir);

	return mVideoNodes.size();
}

int CameraEnumerator::openDevice(const std::string name)
{
	struct stat st;

	if (stat(name.c_str(), &st) < 0) {
		LOG(mLog, ERROR) << "Cannot stat " << name <<
			" video device: " << strerror(errno);
		return -1;
	}

	if (!S_ISCHR(st.st_mode)) {
		LOG(mLog, ERROR) << name << " is not a character device";
		return -1;
	}

	int fd = open(name.c_str(), O_RDWR /* required */ | O_NONBLOCK, 0);

	if (fd < 0) {
		LOG(mLog, ERROR) << "Cannot open " << name <<
			" video device: " << strerror(errno);
		return -1;
	}

	return fd;
}

void CameraEnumerator::closeDevice(const int fd)
{
	close(fd);
}

int CameraEnumerator::isCaptureDevice(int fd, const std::string name)
{
	struct v4l2_capability cap = {0};

	if (xioctl(fd, VIDIOC_QUERYCAP, &cap) < 0) {
		if (EINVAL == errno) {
			LOG(mLog, DEBUG) << name << " is not a V4L2 device";
			return -1;
		} else {
			LOG(mLog, ERROR) <<"Failed to call [VIDIOC_QUERYCAP] for device " << name;
			return -1;
		}
	}

	if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
		LOG(mLog, DEBUG) << name << " is not a video capture device";
		return -1;
	}

	if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
		LOG(mLog, DEBUG) << name << " does not support streaming IO";
		return -1;
	}

	/* FIXME: skip all devices which report 0 width/height */
	struct v4l2_format fmt = {0};

	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (xioctl(fd, VIDIOC_G_FMT, &fmt) < 0) {
		LOG(mLog, ERROR) <<
			"Failed to call [VIDIOC_G_FMT] for device " << name;
		return -1;
	}

	if (!fmt.fmt.pix.width || !fmt.fmt.pix.height) {
		LOG(mLog, DEBUG) << name << " has zero resolution";
		return -1;
	}

	LOG(mLog, DEBUG) << name << " is a valid capture device";

	LOG(mLog, DEBUG) << "\tDriver:   " << cap.driver;
	LOG(mLog, DEBUG) << "\tCard:     " << cap.card;
	LOG(mLog, DEBUG) << "\tBus info: " << cap.bus_info;

	return 0;
}

int CameraEnumerator::enumerateCaptureDevices()
{
	mCaptureDevices.clear();

	for (auto const& node: mVideoNodes) {
		int fd = openDevice(node);

		if (fd < 0)
			continue;

		int ret = isCaptureDevice(fd, node);

		closeDevice(fd);

		if (ret < 0)
			continue;

		mCaptureDevices.push_back(node);
	}

	return mCaptureDevices.size();
}

int CameraEnumerator::init()
{
	int numNodes = enumerateVideoNodes();

	if (numNodes < 0)
		throw XenBackend::Exception("There is no video nodes in /dev",
					    ENODEV);

	LOG(mLog, DEBUG) << "Found " << numNodes << " video device nodes";

	int numCapDevices = enumerateCaptureDevices();

	if (numCapDevices < 0)
		throw XenBackend::Exception("There is no video capture devices",
					    ENODEV);

	LOG(mLog, DEBUG) << "Found " << numCapDevices <<
		" video capture device(s)";

	return 0;
}

void CameraEnumerator::release()
{
}
