// SPDX-License-Identifier: GPL-2.0

/*
 * Xen para-virtualized camera backend
 *
 * Copyright (C) 2018 EPAM Systems Inc.
 */

#include <xen/be/Exception.hpp>

#include "CameraManager.hpp"

using XenBackend::Exception;

CameraManager::CameraManager():
	mLog("CameraManager")
{
}

CameraManager::~CameraManager()
{
}

CameraPtr CameraManager::getCamera(std::string uniqueId)
{
	auto it = mCameraList.find(uniqueId);

	if (it != mCameraList.end())
		return it->second;

	/* This camera is not on the list yet - create now. */
	try {
		auto camera =
			CameraPtr(new Camera(uniqueId,
					     Camera::eAllocMode::ALLOC_DMABUF));
		mCameraList[uniqueId] = camera;
	} catch (Exception &e) {
		throw Exception("Failed to initialize Camera with unique-id " +
				uniqueId, errno);
	}

	return 	mCameraList[uniqueId];
}

