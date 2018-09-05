
// SPDX-License-Identifier: GPL-2.0

/*
 * Xen para-virtualized camera backend
 *
 * Copyright (C) 2018 EPAM Systems Inc.
 */

#include "CommandHandler.hpp"

#include <algorithm>
#include <iomanip>

#include <xen/be/Exception.hpp>

using std::hex;
using std::setfill;
using std::setw;
using std::unordered_map;

unordered_map<int, CommandHandler::CommandFn> CommandHandler::sCmdTable =
{
    { XENCAMERA_OP_SET_CONFIG,          &CommandHandler::setConfig },
    { XENCAMERA_OP_SET_CTRL,            &CommandHandler::setCtrl },
    { XENCAMERA_OP_GET_CTRL_DETAILS,    &CommandHandler::getCtrlDetails },

};

/*******************************************************************************
 * CamCtrlRingBuffer
 ******************************************************************************/
CtrlRingBuffer::CtrlRingBuffer(EventRingBufferPtr eventBuffer,
                               domid_t domId, evtchn_port_t port,
                               grant_ref_t ref,
                               std::string ctrls,
                               CameraPtr camera) :
    RingBufferInBase<xen_cameraif_back_ring, xen_cameraif_sring,
                     xencamera_req, xencamera_resp>(domId, port, ref),
    mCommandHandler(eventBuffer, ctrls, camera),
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

CommandHandler::CommandHandler(EventRingBufferPtr eventBuffer,
                               std::string ctrls,
                               CameraPtr camera) :
    mEventBuffer(eventBuffer),
    mEventId(0),
    mCamera(camera),
    mLog("CommandHandler")
{
    LOG(mLog, DEBUG) << "Create command handler";

    try {
        init(ctrls);
    } catch (...) {
        release();
        throw;
    }
}

CommandHandler::~CommandHandler()
{
    LOG(mLog, DEBUG) << "Delete command handler";
}

void CommandHandler::init(std::string ctrls)
{
    std::stringstream ss(ctrls);
    std::string item;
    while (std::getline(ss, item, XENCAMERA_LIST_SEPARATOR[0]))
        mCameraControls.push_back(CameraControl {
                                  .name = item,
                                  .v4l2_cid = -1});
}

void CommandHandler::release()
{
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

int CommandHandler::toXenControlType(int v4l2_cid)
{
    if (v4l2_cid == V4L2_CID_CONTRAST)
        return XENCAMERA_CTRL_CONTRAST;

    if (v4l2_cid == V4L2_CID_BRIGHTNESS)
        return XENCAMERA_CTRL_BRIGHTNESS;

    if (v4l2_cid == V4L2_CID_HUE)
        return XENCAMERA_CTRL_HUE;

    if (v4l2_cid == V4L2_CID_SATURATION)
        return XENCAMERA_CTRL_SATURATION;

    throw XenBackend::Exception("Unsupported V4L2 CID " +
                                std::to_string(v4l2_cid), EINVAL);
}

int CommandHandler::fromXenControlType(int xen_type)
{
    if (xen_type == XENCAMERA_CTRL_CONTRAST)
        return V4L2_CID_CONTRAST;

    if (xen_type == XENCAMERA_CTRL_BRIGHTNESS)
        return V4L2_CID_BRIGHTNESS;

    if (xen_type == XENCAMERA_CTRL_HUE)
        return V4L2_CID_HUE;

    if (xen_type == XENCAMERA_CTRL_SATURATION)
        return V4L2_CID_SATURATION;

    throw XenBackend::Exception("Unsupported Xen control type " +
                                std::to_string(xen_type), EINVAL);
}

void CommandHandler::getCtrlDetails(const xencamera_req& req,
                                    xencamera_resp& resp)
{
    const xencamera_get_ctrl_details_req *ctrlDetReq = &req.req.get_ctrl_details;

    DLOG(mLog, DEBUG) << "Handle command [GET CTRL DETAILS]";

    if (ctrlDetReq->index >= mCameraControls.size())
        throw XenBackend::Exception("Control " +
                                    std::to_string(ctrlDetReq->index) +
                                    " is not assigned", EINVAL);

    auto details = mCamera->getControlDetails(
        mCameraControls[ctrlDetReq->index].name);

    mCameraControls[ctrlDetReq->index].v4l2_cid = details.v4l2_cid;
    resp.resp.ctrl_details.index = ctrlDetReq->index;
    resp.resp.ctrl_details.type = toXenControlType(details.v4l2_cid);
    resp.resp.ctrl_details.min = details.minimum;
    resp.resp.ctrl_details.max = details.maximum;
    resp.resp.ctrl_details.step = details.step;
    resp.resp.ctrl_details.def_val = details.default_value;
}

void CommandHandler::setCtrl(const xencamera_req& req, xencamera_resp& resp)
{
    const xencamera_set_ctrl_req *ctrlSetReq = &req.req.set_ctrl;

    DLOG(mLog, DEBUG) << "Handle command [SET CTRL]";

    int v4l2_cid = fromXenControlType(ctrlSetReq->type);

    auto it = std::find_if(mCameraControls.begin(), mCameraControls.end(),
                           [v4l2_cid](const CameraControl& item) {
                               if (item.v4l2_cid == v4l2_cid)
                                   return true;

                               return false;
                           });

    if (it == mCameraControls.end()) {
        std::stringstream stream;

        stream << std::hex << v4l2_cid;
        throw XenBackend::Exception("Control with V4L2 CID " +
                                    stream.str() +
                                    " is not assigned", EINVAL);
    }

    mCamera->setControl(v4l2_cid, ctrlSetReq->value);
}

