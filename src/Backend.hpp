/* SPDX-License-Identifier: GPL-2.0 */

/*
 * Xen para-virtualized camera backend
 *
 * Copyright (C) 2018 EPAM Systems Inc.
 */

#ifndef SRC_BACKEND_HPP_
#define SRC_BACKEND_HPP_

#include <list>

#include "Camera.hpp"

#include <xen/be/Log.hpp>

class Backend
{
public:
	Backend();

	void start();

private:
	XenBackend::Log mLog;

	std::list<CameraPtr> mCameraList;
};

#endif /* SRC_BACKEND_HPP_ */
