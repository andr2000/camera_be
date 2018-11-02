
// SPDX-License-Identifier: GPL-2.0

/*
 * Xen para-virtualized camera backend
 *
 * Copyright (C) 2018 EPAM Systems Inc.
 */

#include "CommandHandler.hpp"
#include "V4L2ToXen.hpp"

#include <algorithm>
#include <iomanip>

#include <xen/be/Exception.hpp>

using std::hex;
using std::setfill;
using std::setw;
using std::unordered_map;

unordered_map<int, CommandHandler::CommandFn> CommandHandler::sCmdTable =
{
    { XENCAMERA_OP_CONFIG_SET,          &CommandHandler::configSet },
    { XENCAMERA_OP_CONFIG_GET,          &CommandHandler::configGet },
    { XENCAMERA_OP_BUF_GET_LAYOUT,      &CommandHandler::bufGetLayout },
    { XENCAMERA_OP_BUF_REQUEST,         &CommandHandler::bufRequest },
    { XENCAMERA_OP_CTRL_SET,            &CommandHandler::ctrlSet },
    { XENCAMERA_OP_CTRL_ENUM,           &CommandHandler::ctrlEnum },
    { XENCAMERA_OP_CTRL_ENUM,           &CommandHandler::ctrlEnum },
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

void CommandHandler::configToXen(xencamera_config *cfg)
{
    v4l2_format fmt = mCamera->getFormat();

    cfg->pixel_format = fmt.fmt.pix.pixelformat;
    cfg->width = fmt.fmt.pix.width;
    cfg->height = fmt.fmt.pix.height;

    cfg->colorspace = V4L2ToXen::colorspaceToXen(fmt.fmt.pix.colorspace);

    cfg->xfer_func = V4L2ToXen::xferToXen(fmt.fmt.pix.xfer_func);

    cfg->ycbcr_enc = V4L2ToXen::ycbcrToXen(fmt.fmt.pix.ycbcr_enc);

    cfg->quantization = V4L2ToXen::quantizationToXen(fmt.fmt.pix.quantization);

    cfg->displ_asp_ratio_numer = 1;
    cfg->displ_asp_ratio_denom = 1;

    v4l2_fract frameRate = mCamera->getFrameRate();

    cfg->frame_rate_numer = frameRate.numerator;
    cfg->frame_rate_denom = frameRate.denominator;
}

void CommandHandler::configSet(const xencamera_req& req,
                               xencamera_resp& resp)
{
    const xencamera_config *cfgReq = &req.req.config;

    DLOG(mLog, DEBUG) << "Handle command [CONFIG SET]";

    v4l2_format fmt {0};

    fmt.fmt.pix.pixelformat = cfgReq->pixel_format;
    fmt.fmt.pix.width = cfgReq->width;
    fmt.fmt.pix.height = cfgReq->height;

    fmt.fmt.pix.colorspace = V4L2ToXen::colorspaceToV4L2(cfgReq->colorspace);

    fmt.fmt.pix.xfer_func = V4L2ToXen::xferToV4L2(cfgReq->xfer_func);

    fmt.fmt.pix.ycbcr_enc = V4L2ToXen::ycbcrToV4L2(cfgReq->ycbcr_enc);

    fmt.fmt.pix.quantization = V4L2ToXen::quantizationToV4L2(cfgReq->quantization);

    mCamera->setFormat(fmt);

    mCamera->setFrameRate(cfgReq->frame_rate_numer, cfgReq->frame_rate_denom);

    configToXen(&resp.resp.config);
}

void CommandHandler::configGet(const xencamera_req& req,
                               xencamera_resp& resp)
{
    DLOG(mLog, DEBUG) << "Handle command [CONFIG GET]";

    configToXen(&resp.resp.config);
}

void CommandHandler::bufGetLayout(const xencamera_req& req,
                                  xencamera_resp& resp)
{
    xencamera_buf_get_layout_resp *bufLayoutResp = &resp.resp.buf_layout;

    DLOG(mLog, DEBUG) << "Handle command [BUF GET LAYOUT]";

    v4l2_format fmt = mCamera->getFormat();

    DLOG(mLog, DEBUG) << "Handle command [BUF GET LAYOUT] size " <<
        fmt.fmt.pix.sizeimage;

    bufLayoutResp->num_planes = 1;
    bufLayoutResp->size = fmt.fmt.pix.sizeimage;
    bufLayoutResp->plane_offset[0] = 0;
    bufLayoutResp->plane_size[0] = fmt.fmt.pix.sizeimage;
    bufLayoutResp->plane_stride[0] = fmt.fmt.pix.bytesperline;
}

void CommandHandler::bufRequest(const xencamera_req& req,
                                xencamera_resp& resp)
{
    const xencamera_buf_request *bufRequestReq = &req.req.buf_request;
    xencamera_buf_request *bufRequestResp = &resp.resp.buf_request;

    DLOG(mLog, DEBUG) << "Handle command [BUF REQUEST]";

    bufRequestResp->num_bufs =
        mCamera->requestBuffers(bufRequestReq->num_bufs);

    DLOG(mLog, DEBUG) << "Handle command [BUF REQUEST] num_bufs " <<
        std::to_string(bufRequestResp->num_bufs);
}

void CommandHandler::ctrlEnum(const xencamera_req& req,
                              xencamera_resp& resp)
{
    const xencamera_index *ctrlEnumReq = &req.req.index;
    xencamera_ctrl_enum_resp *ctrlEnumResp = &resp.resp.ctrl_enum;

    DLOG(mLog, DEBUG) << "Handle command [CTRL ENUM]";

    if (ctrlEnumReq->index >= mCameraControls.size())
        throw XenBackend::Exception("Control " +
                                    std::to_string(ctrlEnumReq->index) +
                                    " is not assigned", EINVAL);

    auto details = mCamera->getControlDetails(
        mCameraControls[ctrlEnumReq->index].name);

    mCameraControls[ctrlEnumReq->index].v4l2_cid = details.v4l2_cid;
    ctrlEnumResp->index = ctrlEnumReq->index;
    ctrlEnumResp->type = V4L2ToXen::ctrlToXen(details.v4l2_cid);
    ctrlEnumResp->flags = V4L2ToXen::ctrlFlagsToXen(details.flags);
    ctrlEnumResp->min = details.minimum;
    ctrlEnumResp->max = details.maximum;
    ctrlEnumResp->step = details.step;
    ctrlEnumResp->def_val = details.default_value;
}

void CommandHandler::ctrlSet(const xencamera_req& req, xencamera_resp& resp)
{
    const xencamera_ctrl_value *ctrlSetReq = &req.req.ctrl_value;

    DLOG(mLog, DEBUG) << "Handle command [SET CTRL]";

    int v4l2_cid = V4L2ToXen::ctrlToV4L2(ctrlSetReq->type);

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

    /* TODO: propagate this event to other frontends other than this one. */
}

