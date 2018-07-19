/* SPDX-License-Identifier: GPL-2.0 */

/*
 * Xen para-virtualized camera backend
 *
 * Copyright (C) 2018 EPAM Systems Inc.
 */
#ifndef SRC_DEVICEMMAP_HPP_
#define SRC_DEVICEMMAP_HPP_

#include "Device.hpp"

class DeviceMmap : public Device
{
public:
	DeviceMmap(const std::string devName);

	~DeviceMmap();

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

#endif /* SRC_DEVICEMMAP_HPP_ */
