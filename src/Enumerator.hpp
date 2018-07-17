/* SPDX-License-Identifier: GPL-2.0 */

/*
 * Xen para-virtualized camera backend
 *
 * Copyright (C) 2018 EPAM Systems Inc.
 */

#ifndef SRC_ENUMERATOR_HPP_
#define SRC_ENUMERATOR_HPP_

#include <string>
#include <vector>

#include <xen/be/Log.hpp>

class Enumerator
{
public:
	Enumerator();

	const std::vector<std::string> getCaptureDevices() const {
		return mCaptureDevices;
	}

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

#endif /* SRC_ENUMERATOR_HPP_ */
