#pragma once
#include <vector>
#include <iostream>
#include <vulkan/vulkan.h>
#include <cstring>

//If debug, enable validation layers; if release, don't
#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers= true;
#endif

class vulkanDebugger
{
private:
    const std::vector<const char*> mValidationLayers = 
    {
        "VK_LAYER_KHRONOS_validation"
    };
public:
    bool checkValidationLayerSupport();
};
