// SPDX-License-Identifier: GPL-2.0

/*
 * Xen para-virtualized camera backend
 *
 * Copyright (C) 2018 EPAM Systems Inc.
 */

#include "Backend.hpp"
#include "Enumerator.hpp"

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
	int camIndex = 0;

	while(getXenStore().checkIfExist(camBasePath + to_string(camIndex))) {
		LOG(mLog, DEBUG) << "Found camera: " << camIndex;

		camIndex++;
	}
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
	for (auto const& camera: mCameraList)
		camera.second->stop();
}

void Backend::onNewFrontend(domid_t domId, uint16_t devId)
{
	LOG(mLog, DEBUG) << "New frontend: dom_id " <<
		domId << " dev_id " << devId;
	/*
	 * Read camera unique-id for this devId, then
	 * find the corresponding Camera object and pass it
	 * to new CameraFrontendHandler
	 */
	string uniqueId = "video8";

	auto it = mCameraList.find(uniqueId);
	if (it == mCameraList.end())
		throw Exception("Camera with unique-id " + uniqueId +
			" not found", ENOTTY);

	addFrontendHandler(FrontendHandlerPtr(
		new CameraFrontendHandler(it->second, getDeviceName(),
					  getDomId(), domId, devId)));
}

void Backend::init()
{
	Enumerator enumerator;

	for (auto const& devName: enumerator.getCaptureDevices()) {
		int idx = devName.find_last_of('/');
		string uniqueId = devName.substr(idx + 1);

		LOG(mLog, DEBUG) << "Adding new camera at " << devName <<
			", assigning unique ID to " << uniqueId;

		mCameraList[uniqueId] = CameraPtr(new Camera(devName,
			Camera::eAllocMode::ALLOC_DMABUF));
	}

#if 0
	for (auto const& camera: mCameraList)
		camera.second->start();
#endif
}

void Backend::release()
{
}
