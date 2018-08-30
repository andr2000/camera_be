// SPDX-License-Identifier: GPL-2.0

/*
 * Xen para-virtualized camera backend
 *
 * Copyright (C) 2018 EPAM Systems Inc.
 */

#include "Backend.hpp"
#include "CommandHandler.hpp"

#include <xen/be/Exception.hpp>

using std::to_string;
using std::string;

using XenBackend::Exception;
using XenBackend::FrontendHandlerPtr;

/*******************************************************************************
 * CameraFrontendHandler
 ******************************************************************************/

void CameraFrontendHandler::onBind()
{
    LOG(mLog, DEBUG) << "On frontend bind : " << getDomId();

    string camBasePath = getXsFrontendPath() + "/";

    auto evt_port = getXenStore().readInt(camBasePath +
                                          XENCAMERA_FIELD_EVT_CHANNEL);

    auto evt_ref = getXenStore().readInt(camBasePath +
                                         XENCAMERA_FIELD_EVT_RING_REF);

    auto req_port = getXenStore().readInt(camBasePath +
                                          XENCAMERA_FIELD_REQ_CHANNEL);

    auto req_ref = getXenStore().readInt(camBasePath +
                                         XENCAMERA_FIELD_REQ_RING_REF);

    auto uniqueId =  getXenStore().readString(camBasePath +
                                              XENCAMERA_FIELD_UNIQUE_ID);

    mCamera = mCameraManager->getCamera(uniqueId);

    EventRingBufferPtr eventRingBuffer(new EventRingBuffer(getDomId(),
                                                           evt_port,
                                                           evt_ref,
                                                           XENCAMERA_IN_RING_OFFS,
                                                           XENCAMERA_IN_RING_SIZE));

    addRingBuffer(eventRingBuffer);

    CtrlRingBufferPtr ctrlRingBuffer(new CtrlRingBuffer(eventRingBuffer,
                                                        getDomId(),
                                                        req_port,
                                                        req_ref));

    addRingBuffer(ctrlRingBuffer);


    mCamera->start();
}


class dddd {
public:
};

/*******************************************************************************
 * CameraBackend
 ******************************************************************************/

Backend::Backend(const string& deviceName) :
    BackendBase("CameraBackend", deviceName),
    mLog("CameraBackend")

{
    try {
        init();
    } catch (...) {
        release();
        throw;
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
