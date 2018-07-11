/* SPDX-License-Identifier: GPL-2.0 */

/*
 * Xen para-virtualized camera backend
 *
 * Copyright (C) 2018 EPAM Systems Inc.
 */

#include <xen/be/Log.hpp>

class CameraBackend
{
public:
	CameraBackend();

	void start();

private:
	XenBackend::Log mLog;
};
