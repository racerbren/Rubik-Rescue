#pragma once
#include <iostream>
#include <SDL.h>
#include <SDL_vulkan.h>
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>
#include <vector>
#include <map>
#include <optional>
#include <set>
#include <limits>
#include <algorithm>
#include <fstream>

#include "vulkanDebugger.h"

#define VK_USE_PLATFORM_WIN32_KHR

const uint32_t SCREEN_WIDTH = 1080;
const uint32_t SCREEN_HEIGHT = 720;

struct QueueFamilyIndices
{
    //std::optional contains no value until we assign one to it. This is useful in case a queue family is unavailable
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete()
    {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

struct SwapChainSupportDetails 
{
    //Surface capabilities such as min/max number of images in swap chain or min/max width/height of images)
    VkSurfaceCapabilitiesKHR capabilities;

    //Surface formats including pixel format and color space
    std::vector<VkSurfaceFormatKHR> formats;

    //Available presentation modes
    std::vector<VkPresentModeKHR> presentModes;
};

class renderApp 
{
private:
    SDL_Window* mWindow = NULL;
    VkInstance mInstance;
    vulkanDebugger mDebugger;
    VkDevice mDevice;
    VkPhysicalDevice mPhysicalDevice = VK_NULL_HANDLE;
    VkQueue mGraphicsQueue;
    VkSurfaceKHR mSurface;
    VkQueue mPresentQueue;
    VkSwapchainKHR mSwapChain;
    std::vector<VkImage> mSwapChainImages;
    VkFormat mSwapChainImageFormat;
    VkExtent2D mSwapChainExtent;
    std::vector<VkImageView> mSwapChainImageViews;
    VkRenderPass mRenderPass;
    VkPipelineLayout mPipelineLayout;
    VkPipeline mGraphicsPipeline;
    std::vector<VkFramebuffer> mSwapChainFramebuffers;
    VkCommandPool mCommandPool;
    VkCommandBuffer mCommandBuffer;
    VkSemaphore swapchainSemaphore;
    VkSemaphore renderingSemaphore;
    VkFence inFlightFence;

    const std::vector<const char*> mDeviceExtensions = 
    {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    void initWindow();
    void initVulkan();
    void loop();
    void clean();

    void createInstance();
    void pickPhysicalDevice();
    int rateDeviceSuitability(VkPhysicalDevice device);
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
    void createLogicalDevice();
    void createSurface();
    bool checkDeviceExtensionSupport(VkPhysicalDevice device);
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);\
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);    //Surface format (color depth)
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);     //Presentation mode (conditions for swapping images)
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);                              //Swap extent (resolution of swap chain images)
    void createSwapChain();
    void createImageViews();
    void createGraphicsPipeline();
    static std::vector<char> readFile(const std::string& filename);
    VkShaderModule createShaderModule(const std::vector<char>& code);
    void createRenderPass();
    void createFramebuffers();
    void createCommandPool();
    void createCommandBuffer();
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
    void drawFrame();
    void createSyncObjects();
public:
    void run();
};