
#include "RQueue.h"
#include "RLogicalDevice.h"
#include "RLogger.h"

namespace rtvvulfw {

RQueue::RQueue(RLogicalDevice *logicalDevice, uint32_t queueIndex) : mLogicalDevice{ logicalDevice } {
    vkGetDeviceQueue(logicalDevice->handle(), queueIndex, 0, &mQueue);
    if (mQueue == VK_NULL_HANDLE) {
        LOG_ERROR(tag(), "Could not get queue handle");
        throw std::runtime_error("Could not get queue handle");
    }

    LOG_DEBUG(tag(), "Queue Created : " << queueIndex);
}

RQueue::~RQueue() {
}

}