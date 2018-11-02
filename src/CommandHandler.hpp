/* SPDX-License-Identifier: GPL-2.0 */

/*
 * Xen para-virtualized camera backend
 *
 * Copyright (C) 2018 EPAM Systems Inc.
 */
#ifndef SRC_COMMANDHANDLER_HPP_
#define SRC_COMMANDHANDLER_HPP_

#include <cstdint>
#include <unordered_map>
#include <vector>

#include <xen/be/RingBufferBase.hpp>
#include <xen/be/Log.hpp>

#include <xen/io/cameraif.h>

#include "Camera.hpp"

/***************************************************************************//**
 * Ring buffer used to send events to the frontend.
 ******************************************************************************/
class EventRingBuffer : public XenBackend::RingBufferOutBase<
                        xencamera_event_page, xencamera_evt>
{
public:
    /**
     * @param domId     frontend domain id
     * @param port      event channel port number
     * @param ref       grant table reference
     * @param offset    start of the ring buffer inside the page
     * @param size      size of the ring buffer
     */
    EventRingBuffer(domid_t domId, evtchn_port_t port,
                    grant_ref_t ref, int offset, size_t size);

private:
    XenBackend::Log mLog;
};

typedef std::shared_ptr<EventRingBuffer> EventRingBufferPtr;

class CommandHandler
{
public:
    CommandHandler(EventRingBufferPtr eventBuffer, std::string ctrls,
                   CameraPtr camera);
    ~CommandHandler();

    int processCommand(const xencamera_req& req, xencamera_resp& resp);

private:
    typedef void(CommandHandler::*CommandFn)(const xencamera_req& req,
                                             xencamera_resp& resp);

    static std::unordered_map<int, CommandFn> sCmdTable;

    EventRingBufferPtr mEventBuffer;
    uint16_t mEventId;

    CameraPtr mCamera;

    struct CameraControl
    {
        std::string name;
        int v4l2_cid;
    };

    std::vector<CameraControl> mCameraControls;

    XenBackend::Log mLog;

    void init(std::string ctrls);
    void release();

    void configToXen(xencamera_config *cfg);

    void configSet(const xencamera_req& req, xencamera_resp& resp);
    void configGet(const xencamera_req& req, xencamera_resp& resp);

    void bufGetLayout(const xencamera_req& req, xencamera_resp& resp);
    void bufRequest(const xencamera_req& req, xencamera_resp& resp);
    void bufCreate(const xencamera_req& req, xencamera_resp& resp);
    void bufQueue(const xencamera_req& req, xencamera_resp& resp);
    void bufDequeue(const xencamera_req& req, xencamera_resp& resp);
    void bufDestroy(const xencamera_req& req, xencamera_resp& resp);

    void ctrlSet(const xencamera_req& req, xencamera_resp& resp);
    void ctrlEnum(const xencamera_req& req, xencamera_resp& resp);
};

/***************************************************************************//**
 * Ring buffer used for the camera control.
 ******************************************************************************/
class CtrlRingBuffer : public XenBackend::RingBufferInBase<xen_cameraif_back_ring,
    xen_cameraif_sring, xencamera_req, xencamera_resp>
{
public:
    CtrlRingBuffer(EventRingBufferPtr eventBuffer, domid_t domId,
                   evtchn_port_t port, grant_ref_t ref,
                   std::string ctrls, CameraPtr camera);

private:
    CommandHandler mCommandHandler;
    XenBackend::Log mLog;

    virtual void processRequest(const xencamera_req& req) override;
};

typedef std::shared_ptr<CtrlRingBuffer> CtrlRingBufferPtr;

#endif /* SRC_COMMANDHANDLER_HPP_ */
