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

class renderApp 
{
private:
    SDL_Window* mWindow = NULL;
    VkInstance mInstance;
    vulkanDebugger mDebugger;
    VkDevice mDevice;
    VkPhysicalDevice mPhysicalDevice;
    VkQueue mGraphicsQueue;
    VkSurfaceKHR mSurface;
    VkQueue mPresentQueue;

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
public:
    void run();
};