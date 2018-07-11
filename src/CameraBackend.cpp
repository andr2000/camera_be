// SPDX-License-Identifier: GPL-2.0

/*
 * Xen para-virtualized camera backend
 *
 * Copyright (C) 2018 EPAM Systems Inc.
 */

#include "CameraBackend.hpp"
#include "CameraEnumerator.hpp"

CameraBackend::CameraBackend():
	mLog("CameraBackend")
{
}

void CameraBackend::start()
{
	CameraEnumerator enumerator;
}


