/* SPDX-License-Identifier: GPL-2.0 */

/*
 * Xen para-virtualized camera backend
 *
 * Copyright (C) 2018 EPAM Systems Inc.
 */
#ifndef SRC_CAMERADMABUF_HPP_
#define SRC_CAMERADMABUF_HPP_

#include "CameraMmap.hpp"

class CameraDmabuf : public CameraMmap
{
public:
    CameraDmabuf(const std::string devName);

    ~CameraDmabuf();

    void allocStream(int numBuffers, uint32_t width,
                     uint32_t height, uint32_t pixelFormat) override;
    void releaseStream() override;

private:
    std::vector<int> mBufferFds;
};

#endif /* SRC_CAMERADMABUF_HPP_ */
