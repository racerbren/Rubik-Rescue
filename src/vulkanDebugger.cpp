#include "vulkanDebugger.h"

bool vulkanDebugger::checkValidationLayerSupport()
{
    //Find all available layers and add them to a vector
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
    
    //Check if all of the layers in the global validationLayers vector exist in the availableLayers vector
    for (const char* layerName : mValidationLayers) 
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