// SPDX-License-Identifier: GPL-2.0

/*
 * Xen para-virtualized camera backend
 *
 * Copyright (C) 2018 EPAM Systems Inc.
 */

#include <xen/be/Exception.hpp>

#include "CameraManager.hpp"

using std::mutex;
using std::lock_guard;

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
    lock_guard<mutex> lock(mLock);

    auto it = mCameraList.find(uniqueId);

    if (it != mCameraList.end())
        if (auto camera = it->second.lock())
            return camera;

    /* This camera is not on the list yet - create now. */
    auto camera =
        CameraPtr(new Camera(uniqueId,
                             Camera::eAllocMode::ALLOC_DMABUF));
    mCameraList[uniqueId] = camera;

    return camera;
}

