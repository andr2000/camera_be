/* SPDX-License-Identifier: GPL-2.0 */

/*
 * Xen para-virtualized camera backend
 *
 * Copyright (C) 2018 EPAM Systems Inc.
 */

#ifndef CAMERA_ENUMERATOR_HPP_
#define CAMERA_ENUMERATOR_HPP_

#include <string>
#include <vector>

#include <xen/be/Log.hpp>

class CameraEnumerator
{
public:
	CameraEnumerator();

private:
	XenBackend::Log mLog;

	/*
	 * After enumeration this list will hold all available /dev/videoX
	 * device's names.
	 */
	std::vector<std::string> mVideoNodes;

	std::vector<std::string> mCaptureDevices;

	int xioctl(int fd, int request, void *arg);

	int enumerateVideoNodes();

	int openDevice(const std::string name);

	void closeDevice(const int fd);

	int isCaptureDevice(int fd, const std::string name);

	int enumerateCaptureDevices();

	int init();

	void release();
};

#endif /* CAMERA_ENUMERATOR_HPP_ */
