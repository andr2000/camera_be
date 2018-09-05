/* SPDX-License-Identifier: GPL-2.0 */

/*
 * Xen para-virtualized camera backend
 *
 * Copyright (C) 2018 EPAM Systems Inc.
 */
#ifndef SRC_CAMERA_HPP_
#define SRC_CAMERA_HPP_

#include <list>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include <linux/videodev2.h>

#include <xen/be/Log.hpp>
#include <xen/be/Utils.hpp>

#ifdef WITH_DBG_DISPLAY
#include "wayland/Display.hpp"
#endif

class Camera
{
public:
    Camera(const std::string devName);

    ~Camera();

    const std::string getName() const {
        return mDevPath;
    }

    const std::string getUniqueId() const {
        return mDevUniqueId;
    }

    v4l2_format getCurrentFormat() const {
        return mCurFormat;
    }

    virtual void allocStream(int numBuffers, uint32_t width,
                             uint32_t height, uint32_t pixelFormat) = 0;
    virtual void releaseStream() = 0;

    virtual void *getBufferData(int index) = 0;

    typedef std::function<void(int, int)> FrameDoneCallback;

    void startStream(FrameDoneCallback clb);
    void stopStream();

    struct ControlDetails {
        int v4l2_cid;
        signed int minimum;
        signed int maximum;
        signed int default_value;
        signed int step;
    };

    ControlDetails getControlDetails(std::string name);
    void setControl(int v4l2_cid, signed int value);

protected:
    struct FormatSize {
        int width;
        int height;
        std::vector<v4l2_fract> fps;
    };

    struct Format {
        uint32_t pixelFormat;
        std::string description;

        std::vector<FormatSize> size;
    };

    static const v4l2_buf_type cV4L2BufType = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    XenBackend::Log mLog;

    std::mutex mLock;

    const std::string mDevUniqueId;
    const std::string mDevPath;

    int mFd;

    bool mStreamStarted;

    std::vector<std::string> mVideoNodes;

    v4l2_format mCurFormat;
    uint32_t mCurMemoryType;

    std::vector<Format> mFormats;

    std::thread mThread;

    std::unique_ptr<XenBackend::PollFd> mPollFd;

    FrameDoneCallback mFrameDoneCallback;

    void init();
    void release();

    int xioctl(int request, void *arg);

    bool isOpen();
    void openCamera();
    void closeCamera();
    bool isCaptureDevice();

    void getSupportedFormats();
    void printSupportedFormats();
    v4l2_format getFormat();

    int getFrameSize(int index, uint32_t pixelFormat,
                     v4l2_frmsizeenum &size);

    int getFrameInterval(int index, uint32_t pixelFormat,
                         uint32_t width, uint32_t height,
                         v4l2_frmivalenum &interval);

    static float toFps(const v4l2_fract &fract) {
        return static_cast<float>(fract.denominator) / fract.numerator;
    }

    void setFormat(uint32_t width, uint32_t height, uint32_t pixelFormat);

    int requestBuffers(int numBuffers, uint32_t memory);

    v4l2_buffer queryBuffer(int index);

    void queueBuffer(int index);

    v4l2_buffer dequeueBuffer();

    int exportBuffer(int index);

    std::vector<ControlDetails> mControlDetails;

    void enumerateControls();

    void eventThread();

#ifdef WITH_DBG_DISPLAY
    static const int cNumDisplayBuffers = 1;

    DisplayItf::DisplayPtr mDisplay;
    DisplayItf::ConnectorPtr mConnector;
    std::vector<DisplayItf::DisplayBufferPtr> mDisplayBuffer;
    std::vector<DisplayItf::FrameBufferPtr> mFrameBuffer;
    int mCurrentFrameBuffer;

    void initDisplay();
    void releaseDisplay();
    void startDisplay();
    void stopDisplay();
    void displayOnFrameDoneCallback(int index, int size);
#endif
};

typedef std::shared_ptr<Camera> CameraPtr;
typedef std::weak_ptr<Camera> CameraWeakPtr;

#endif /* SRC_CAMERA_HPP_ */
