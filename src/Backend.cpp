// SPDX-License-Identifier: GPL-2.0

/*
 * Xen para-virtualized camera backend
 *
 * Copyright (C) 2018 EPAM Systems Inc.
 */

#include "Backend.hpp"

#include <xen/be/Exception.hpp>

using std::to_string;
using std::string;

using XenBackend::Exception;
using XenBackend::FrontendHandlerPtr;

/*******************************************************************************
 * CamCtrlRingBuffer
 ******************************************************************************/
CtrlRingBuffer::CtrlRingBuffer(domid_t domId, evtchn_port_t port,
			       grant_ref_t ref) :
	RingBufferInBase<xen_cameraif_back_ring, xen_cameraif_sring,
		xencamera_req, xencamera_resp>(domId, port, ref),
	mLog("CamCtrlRing")
{
	LOG(mLog, DEBUG) << "Create ctrl ring buffer";
}

void CtrlRingBuffer::processRequest(const xencamera_req& req)
{
	DLOG(mLog, DEBUG) << "Request received, cmd:"
		<< static_cast<int>(req.operation);

	xencamera_resp rsp {0};

	sendResponse(rsp);
}

/*******************************************************************************
 * CameraFrontendHandler
 ******************************************************************************/

void CameraFrontendHandler::onBind()
{
	LOG(mLog, DEBUG) << "On frontend bind : " << getDomId();

	string camBasePath = getXsFrontendPath() + "/";
	auto uniqueId =  getXenStore().readString(camBasePath +
						  XENCAMERA_FIELD_UNIQUE_ID);

	mCamera = mCameraManager->getCamera(uniqueId);
	mCamera->start();
}

/*******************************************************************************
 * CameraBackend
 ******************************************************************************/

Backend::Backend(const string& deviceName) :
	BackendBase("CameraBackend", deviceName),
	mLog("CameraBackend")

{
	try {
		init();
	} catch (Exception &e) {
		release();
	}
}

Backend::~Backend()
{
}

void Backend::onNewFrontend(domid_t domId, uint16_t devId)
{
	addFrontendHandler(FrontendHandlerPtr(
		new CameraFrontendHandler(mCameraManager, getDeviceName(),
					  getDomId(), domId, devId)));
}

void Backend::init()
{
	mCameraManager.reset(new CameraManager());
}

void Backend::release()
{
}
