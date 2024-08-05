#pragma once
#include <iostream>
#include <SDL.h>
#include <SDL_vulkan.h>
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>
#include <vector>
#include <map>

#include "vulkanDebugger.h"

const uint32_t SCREEN_WIDTH = 1080;
const uint32_t SCREEN_HEIGHT = 720;

class renderApp 
{
private:
    SDL_Window* mWindow = NULL;
    VkInstance mInstance;
    vulkanDebugger mDebugger;

    void initWindow();
    void initVulkan();
    void loop();
    void clean();

    void createInstance();
    void pickPhysicalDevice();
    int rateDeviceSuitability(VkPhysicalDevice device);
public:
    void run();
};