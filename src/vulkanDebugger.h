#pragma once
#include <vector>
#include <iostream>
#include <vulkan/vulkan.h>
#include <cstring>
#include <SDL.h>
#include <SDL_vulkan.h>

//If debug, enable validation layers; if release, don't
#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers= true;
#endif

class vulkanDebugger
{
public:
    VkDebugUtilsMessengerEXT debugMessenger;
    VkDebugUtilsMessengerCreateInfoEXT createInfo{};

    const std::vector<const char*> validationLayers = 
    {
        "VK_LAYER_KHRONOS_validation"
    };

    bool checkValidationLayerSupport();
    std::vector<const char*> getRequiredExtensions();
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
    void setUpDebugMessenger(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
    VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
    void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);
    
    //Param 1: The severity of the message (diagnostic, warning, invalid, bug)
    //Param 2: How the message relates (performance, possible mistakes, non-optimal use of Vulkan)
    //Param 3: Contains the details of the message
    //Param 4: For user's use to place data in
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) 
    {
        std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

        return VK_FALSE;
    }
};
