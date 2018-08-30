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

/***************************************************************************//**
 * Ring buffer used for the camera control.
 ******************************************************************************/
class CtrlRingBuffer : public XenBackend::RingBufferInBase<xen_cameraif_back_ring,
    xen_cameraif_sring, xencamera_req, xencamera_resp>
{
public:
    CtrlRingBuffer(domid_t domId, evtchn_port_t port, grant_ref_t ref);

private:
    XenBackend::Log mLog;

    virtual void processRequest(const xencamera_req& req) override;
};

typedef std::shared_ptr<CtrlRingBuffer> CtrlRingBufferPtr;

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
    CommandHandler(EventRingBufferPtr eventBuffer);
    ~CommandHandler();

    int processCommand(const xencamera_req& req);

private:
    typedef void(CommandHandler::*CommandFn)(const xencamera_req& req);

    static std::unordered_map<int, CommandFn> sCmdTable;

    EventRingBufferPtr mEventBuffer;
    uint16_t mEventId;

    XenBackend::Log mLog;

    void setConfig(const xencamera_req& req);
};

#endif /* SRC_COMMANDHANDLER_HPP_ */
