/* SPDX-License-Identifier: GPL-2.0 */

/*
 * Xen para-virtualized camera backend
 *
 * Copyright (C) 2018 EPAM Systems Inc.
 */
#ifndef SRC_CAMERAMANGER_HPP_
#define SRC_CAMERAMANGER_HPP_

#include <xen/be/Log.hpp>

#include "Camera.hpp"

class CameraManager
{
public:
    CameraManager();
    ~CameraManager();

    CameraPtr getCamera(std::string uniqueId);

private:
    enum class eAllocMode {
        ALLOC_MMAP,
        ALLOC_USRPTR,
        ALLOC_DMABUF
    };

    XenBackend::Log mLog;
    std::mutex mLock;

    std::unordered_map<std::string, CameraWeakPtr> mCameraList;

    CameraPtr getNewCamera(const std::string devName, eAllocMode mode);
};

typedef std::shared_ptr<CameraManager> CameraManagerPtr;

#endif /* SRC_CAMERAMANGER_HPP_ */
