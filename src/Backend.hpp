/* SPDX-License-Identifier: GPL-2.0 */

/*
 * Xen para-virtualized camera backend
 *
 * Copyright (C) 2018 EPAM Systems Inc.
 */

#ifndef SRC_BACKEND_HPP_
#define SRC_BACKEND_HPP_

#include <list>
#include <string>

#include <xen/be/BackendBase.hpp>
#include <xen/be/FrontendHandlerBase.hpp>
#include <xen/be/RingBufferBase.hpp>
#include <xen/be/Log.hpp>

#include <xen/io/cameraif.h>

#include "CameraManager.hpp"

/***************************************************************************//**
 * Ring buffer used for the camera control.
 ******************************************************************************/
class CtrlRingBuffer : public XenBackend::RingBufferInBase<
	xen_cameraif_back_ring, xen_cameraif_sring, xencamera_req, xencamera_resp>
{
public:
	CtrlRingBuffer(domid_t domId, evtchn_port_t port, grant_ref_t ref);

private:
	XenBackend::Log mLog;

	virtual void processRequest(const xencamera_req& req) override;
};

typedef std::shared_ptr<CtrlRingBuffer> CtrlRingBufferPtr;

/***************************************************************************//**
 * Camera frontend handler.
 ******************************************************************************/
class CameraFrontendHandler : public XenBackend::FrontendHandlerBase
{
public:
	CameraFrontendHandler(CameraManagerPtr cameraManager,
			      const std::string& devName, domid_t beDomId,
			      domid_t feDomId, uint16_t devId) :
		FrontendHandlerBase("CameraFrontend", devName,
				    beDomId, feDomId, devId),
		mLog("CameraFrontend"),
		mCameraManager(cameraManager) {}

protected:
	/**
	 * Is called on connected state when ring buffers binding is required.
	 */
	void onBind() override;

private:
	XenBackend::Log mLog;

	CameraManagerPtr mCameraManager;
	CameraPtr mCamera;
};

class Backend : public XenBackend::BackendBase
{
public:
	Backend(const std::string& deviceName);
	~Backend();

private:
	XenBackend::Log mLog;
	CameraManagerPtr mCameraManager;

	void init();

	void release();

	void onNewFrontend(domid_t domId, uint16_t devId);
};

#endif /* SRC_BACKEND_HPP_ */
