/* SPDX-License-Identifier: GPL-2.0 */

/*
 * Xen para-virtualized camera backend
 *
 * Copyright (C) 2018 EPAM Systems Inc.
 */
#ifndef SRC_CAMERAMMAP_HPP_
#define SRC_CAMERAMMAP_HPP_

#include "Camera.hpp"

class CameraMmap : public Camera
{
public:
    CameraMmap(const std::string devName);

    ~CameraMmap();

    void *getBufferData(int index) override {
        return mBuffers[index].data;
    }

    void allocStream(int numBuffers, uint32_t width,
                     uint32_t height, uint32_t pixelFormat) override;
    void releaseStream() override;

protected:
    struct Buffer {
        size_t size;
        void *data;
    };

    std::vector<Buffer> mBuffers;

    void allocStreamUnlocked(int numBuffers, uint32_t width,
                             uint32_t height, uint32_t pixelFormat);
    void releaseStreamUnlocked();

};

#endif /* SRC_CAMERAMMAP_HPP_ */
