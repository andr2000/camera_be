/* SPDX-License-Identifier: GPL-2.0 */

/*
 * Xen para-virtualized camera backend
 *
 * Copyright (C) 2018 EPAM Systems Inc.
 */
#ifndef SRC_DEVICEDMABUF_HPP_
#define SRC_DEVICEDMABUF_HPP_

#include "DeviceMmap.hpp"

class DeviceDmabuf : public DeviceMmap
{
public:
    DeviceDmabuf(const std::string devName);

    ~DeviceDmabuf();

    void allocStream(int numBuffers, uint32_t width,
                     uint32_t height, uint32_t pixelFormat) override;
    void releaseStream() override;

private:
    std::vector<int> mBufferFds;
};

#endif /* SRC_DEVICEDMABUF_HPP_ */
