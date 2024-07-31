#include "vulkanDebugger.h"

bool vulkanDebugger::checkValidationLayerSupport()
{
    //Find all available layers and add them to a vector
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
    
    //Check if all of the layers in the global validationLayers vector exist in the availableLayers vector
    for (const char* layerName : validationLayers) 
    {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        //If the current layer is not available, return false. Otherwise continue loop
        if (!layerFound) {
            return false;
        }
    }

    //if all of the specified validation layers are available, return true
    return true;
}

std::vector<const char*> vulkanDebugger::getRequiredExtensions()
{
    //Get the names of Vulkan instance extensions required to create an SDL Vulkan Surface
    uint32_t sdlExtensionCount = 0;
    SDL_Vulkan_GetInstanceExtensions(nullptr, &sdlExtensionCount, nullptr);
    std::vector<const char*> sdlExtensions(sdlExtensionCount);
    SDL_Vulkan_GetInstanceExtensions(nullptr, &sdlExtensionCount, sdlExtensions.data());

    //If we are in debug mode, add the debug extension
     if (enableValidationLayers)
        sdlExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    return sdlExtensions;
}

void vulkanDebugger::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& CI)
{
    CI.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT; //Specify the type of create info structure
    CI.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT; //Receive all information about possible problems without general debug info
    CI.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT; //Receive all message types
    CI.pfnUserCallback = debugCallback; //Pointer to callback function
    CI.pUserData = nullptr; // Optional
}

void vulkanDebugger::setUpDebugMessenger(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
{
    if (!enableValidationLayers) return;
    
    populateDebugMessengerCreateInfo(createInfo);

    if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS)
        throw std::runtime_error("Failed to set up debug messenger!");

}

VkResult vulkanDebugger::CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
{
    //Create messenger object throught this proxy function
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr)
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    else
        return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void vulkanDebugger::DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
{

    //Cleanup debug messenger object through another proxy function
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr)
        func(instance, debugMessenger, pAllocator);
}
