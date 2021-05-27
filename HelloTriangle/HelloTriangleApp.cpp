
#include "HelloTriangleApp.h"
#include "RSwapChain.h"
#include "RLogicalDevice.h"
#include "RSwapChain.h"
#include "RQueue.h"

HelloTriangle::HelloTriangle() {
}

HelloTriangle::~HelloTriangle() {
}


void HelloTriangle::init() {
    rvkfw::RApplication::init();

    mRenderPass = std::make_shared<rvkfw::RRenderPass>(logicalDevice(), swapChain());
    mRenderPass->create();

    mFrameBuffer = std::make_shared<rvkfw::RFramebuffer>(mRenderPass, swapChain(), logicalDevice());
    mFrameBuffer->create();

    mGraphicsPipeline = std::make_shared<rvkfw::RGraphicsPipeline>(logicalDevice(), mRenderPass, swapChain());
    mGraphicsPipeline->create(mShaderDirectory + "/ColoredTriangle.vert.spv",
                              mShaderDirectory + "/ColoredTriangle.frag.spv");
    
    mCommandBuffer = std::make_shared<rvkfw::RCommandBuffer>(mRenderPass, swapChain(),
                                                             logicalDevice(), mFrameBuffer, mGraphicsPipeline,
                                                             commandPool());
    mCommandBuffer->create();



    mImageAvailableSemaphores.resize(mMaxFramesInFlight);
    mRenderFinishedSemaphores.resize(mMaxFramesInFlight);
    mInFlightFences.resize(mMaxFramesInFlight);
    mImagesInFlight.resize(swapChain()->images().size(), VK_NULL_HANDLE);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < mMaxFramesInFlight; i++) {
        if (vkCreateSemaphore(logicalDevice()->handle(), &semaphoreInfo, nullptr, &mImageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(logicalDevice()->handle(), &semaphoreInfo, nullptr, &mRenderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(logicalDevice()->handle(), &fenceInfo, nullptr, &mInFlightFences[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create synchronization objects for a frame!");
        }
    }

    recordDrawCommands();
}

void HelloTriangle::recordDrawCommands() {

    auto commandBuffers = mCommandBuffer->commandBuffers();
    for (size_t i = 0; i < commandBuffers.size(); i++) {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0;
        beginInfo.pInheritanceInfo = nullptr;
        if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("failed to begin recording command buffer!");
        }

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = mRenderPass->handle();
        renderPassInfo.framebuffer = mFrameBuffer->frameBufferHandles()[i];
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = swapChain()->swapExtent();

        VkClearValue clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;

        vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, mGraphicsPipeline->handle());

        vkCmdDraw(commandBuffers[i], 3, 1, 0, 0);

        vkCmdEndRenderPass(commandBuffers[i]);

        if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer!");
        }
    }
}

void HelloTriangle::cleanup() {
    for (auto semaphore: mRenderFinishedSemaphores) {
        vkDestroySemaphore(logicalDevice()->handle(), semaphore, nullptr);
    }
    mRenderFinishedSemaphores.clear();

    for (auto semaphore: mImageAvailableSemaphores) {
        vkDestroySemaphore(logicalDevice()->handle(), semaphore, nullptr);
    }
    mImageAvailableSemaphores.clear();

    mCommandBuffer = nullptr;
    mGraphicsPipeline = nullptr;
    mCommandBuffer = nullptr;
    mRenderPass = nullptr;
    mFrameBuffer = nullptr;

    rvkfw::RApplication::cleanup();
}

void HelloTriangle::draw() const {
    vkWaitForFences(logicalDevice()->handle(), 1, &mInFlightFences[mCurrentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    vkAcquireNextImageKHR(logicalDevice()->handle(), swapChain()->handle(), UINT64_MAX, mImageAvailableSemaphores[mCurrentFrame], VK_NULL_HANDLE, &imageIndex);

    if (mImagesInFlight[imageIndex] != VK_NULL_HANDLE) {
        vkWaitForFences(logicalDevice()->handle(), 1, &mImagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
    }
    mImagesInFlight[imageIndex] = mInFlightFences[mCurrentFrame];

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {mImageAvailableSemaphores[mCurrentFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &mCommandBuffer->commandBuffers()[imageIndex];

    VkSemaphore signalSemaphores[] = {mRenderFinishedSemaphores[mCurrentFrame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    vkResetFences(logicalDevice()->handle(), 1, &mInFlightFences[mCurrentFrame]);

    if (vkQueueSubmit(logicalDevice()->graphicsQueue()->handle(), 1, &submitInfo, mInFlightFences[mCurrentFrame]) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = {swapChain()->handle()};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;

    presentInfo.pImageIndices = &imageIndex;

    vkQueuePresentKHR(logicalDevice()->presentQueue()->handle(), &presentInfo);

    mCurrentFrame = (mCurrentFrame + 1) % mMaxFramesInFlight;
}