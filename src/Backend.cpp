// SPDX-License-Identifier: GPL-2.0

/*
 * Xen para-virtualized camera backend
 *
 * Copyright (C) 2018 EPAM Systems Inc.
 */

#include "Backend.hpp"
#include "Enumerator.hpp"

Backend::Backend():
	mLog("Backend")
{
}

Backend::~Backend()
{
	for (auto const& camera: mCameraList)
		camera->stop();
}

void Backend::start()
{
	Enumerator enumerator;

	for (auto const& devName: enumerator.getCaptureDevices()) {
		LOG(mLog, DEBUG) << "Adding new camera at " << devName;

		mCameraList.push_back(CameraPtr(new Camera(devName,
			Camera::eAllocMode::ALLOC_DMABUF)));
	}

	for (auto const& camera: mCameraList)
		camera->start();
}
