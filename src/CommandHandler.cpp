
// SPDX-License-Identifier: GPL-2.0

/*
 * Xen para-virtualized camera backend
 *
 * Copyright (C) 2018 EPAM Systems Inc.
 */

#include "CommandHandler.hpp"

#include <iomanip>

#include <xen/be/Exception.hpp>

using std::hex;
using std::setfill;
using std::setw;
using std::unordered_map;

unordered_map<int, CommandHandler::CommandFn> CommandHandler::sCmdTable =
{
    { XENCAMERA_OP_SET_CONFIG,	        &CommandHandler::setConfig },
    { XENCAMERA_OP_GET_CTRL_DETAILS,    &CommandHandler::getCtrlDetails },

};

/*******************************************************************************
 * CamCtrlRingBuffer
 ******************************************************************************/
CtrlRingBuffer::CtrlRingBuffer(EventRingBufferPtr eventBuffer,
                               domid_t domId, evtchn_port_t port,
                               grant_ref_t ref) :
    RingBufferInBase<xen_cameraif_back_ring, xen_cameraif_sring,
                     xencamera_req, xencamera_resp>(domId, port, ref),
    mCommandHandler(eventBuffer),
    mLog("CamCtrlRing")
{
    LOG(mLog, DEBUG) << "Create ctrl ring buffer";
}

void CtrlRingBuffer::processRequest(const xencamera_req& req)
{
    DLOG(mLog, DEBUG) << "Request received, cmd:"
        << static_cast<int>(req.operation);

    xencamera_resp rsp {0};

    rsp.id = req.id;
    rsp.operation = req.operation;

    rsp.status = mCommandHandler.processCommand(req, rsp);

    sendResponse(rsp);
}

/*******************************************************************************
 * ConEventRingBuffer
 ******************************************************************************/

EventRingBuffer::EventRingBuffer(domid_t domId, evtchn_port_t port,
                                 grant_ref_t ref, int offset, size_t size) :
    RingBufferOutBase<xencamera_event_page, xencamera_evt>(domId, port, ref,
                                                           offset, size),
    mLog("CamEventRing")
{
    LOG(mLog, DEBUG) << "Create event ring buffer";
}

/*******************************************************************************
 * CommandHandler
 ******************************************************************************/

CommandHandler::CommandHandler(
    EventRingBufferPtr eventBuffer) :
    mEventBuffer(eventBuffer),
    mEventId(0),
    mLog("CommandHandler")
{
    LOG(mLog, DEBUG) << "Create command handler";
}

CommandHandler::~CommandHandler()
{
    LOG(mLog, DEBUG) << "Delete command handler";
}

/*******************************************************************************
 * Public
 ******************************************************************************/

int CommandHandler::processCommand(const xencamera_req& req,
                                   xencamera_resp& resp)
{
    int status = 0;

    try
    {
        (this->*sCmdTable.at(req.operation))(req, resp);
    }
    catch(const XenBackend::Exception& e)
    {
        LOG(mLog, ERROR) << e.what();

        status = -e.getErrno();

        if (status >= 0)
        {
            DLOG(mLog, WARNING) << "Positive error code: "
                << static_cast<signed int>(status);

            status = -EINVAL;
        }
    }
    catch(const std::out_of_range& e)
    {
        LOG(mLog, ERROR) << e.what();

        status = -ENOTSUP;
    }
    catch(const std::exception& e)
    {
        LOG(mLog, ERROR) << e.what();

        status = -EIO;
    }

    DLOG(mLog, DEBUG) << "Return status: ["
        << static_cast<signed int>(status) << "]";

    return status;
}

/*******************************************************************************
 * Private
 ******************************************************************************/

void CommandHandler::setConfig(const xencamera_req& req,
                               xencamera_resp& resp)
{
    const xencamera_config *configReq = &req.req.config;

    DLOG(mLog, DEBUG) << "Handle command [SET CONFIG]";
}

void CommandHandler::getCtrlDetails(const xencamera_req& req,
                                    xencamera_resp& resp)
{
    const xencamera_get_ctrl_details_req *ctrlDetReq = &req.req.get_ctrl_details;

    DLOG(mLog, DEBUG) << "Handle command [GET CTRL DETAILS]";

    throw XenBackend::Exception("Control " + std::to_string(ctrlDetReq->index) +
                                " is not assigned", EINVAL);
}

