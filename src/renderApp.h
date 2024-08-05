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

#include "vulkanDebugger.h"

const uint32_t SCREEN_WIDTH = 1080;
const uint32_t SCREEN_HEIGHT = 720;

struct QueueFamilyIndices
{
    //std::optional contains no value until we assign one to it. This is useful in case a queue family is unavailable
    std::optional<uint32_t> graphicsFamily;

    bool isComplete()
    {
        return graphicsFamily.has_value();
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

    void initWindow();
    void initVulkan();
    void loop();
    void clean();

    void createInstance();
    void pickPhysicalDevice();
    int rateDeviceSuitability(VkPhysicalDevice device);
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
    void createLogicalDevice();
public:
    void run();
};