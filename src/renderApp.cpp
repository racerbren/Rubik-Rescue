#include "renderApp.h"

void renderApp::initWindow()
{
    //If you cannot intialize SDL2, exit with error code    
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
        throw std::runtime_error(("Could not initialize SDL! SDL_Error: %s\n", SDL_GetError()));

    mWindow = SDL_CreateWindow("Vulkan Test", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_VULKAN);

    //If you cannot create the window, exit with error code    
    if (mWindow == NULL)
        throw std::runtime_error(("Window could not be created! SDL_Error: %s\n", SDL_GetError()));

    SDL_SetWindowResizable(mWindow, SDL_FALSE);
}

void renderApp::initVulkan()
{
    createInstance();
    mDebugger.setUpDebugMessenger(mInstance, &mDebugger.createInfo, nullptr, &mDebugger.debugMessenger);
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createImageViews();
    createRenderPass();
    createGraphicsPipeline();
    createFramebuffers();
    createCommandPool();
    createCommandBuffer();
    createSyncObjects();
}

void renderApp::loop()
{
    SDL_Event event;
    bool running = true;

    //Main loop
    while (running)
    {
        while (SDL_PollEvent(&event))
            if (event.type == SDL_QUIT)
                running = false;
        drawFrame();
    }
    vkDeviceWaitIdle(mDevice);
}

void renderApp::clean()
{
    //Destroy Semaphores and Fence
    vkDestroySemaphore(mDevice, mSwapchainSemaphore, nullptr);
    vkDestroySemaphore(mDevice, mRenderingSemaphore, nullptr);
    vkDestroyFence(mDevice, mInFlightFence, nullptr);

    //Destroy the command pool
    vkDestroyCommandPool(mDevice, mCommandPool, nullptr);

    //Loop through all framebuffers in the swap chain and destroy them
    for (auto framebuffer : mSwapChainFramebuffers)
        vkDestroyFramebuffer(mDevice, framebuffer, nullptr);

    //Destory the graphics pipeline
    vkDestroyPipeline(mDevice, mGraphicsPipeline, nullptr);

    //Destroy the graphics pipeline layout
    vkDestroyPipelineLayout(mDevice, mPipelineLayout, nullptr);

    //Destroy render pass
    vkDestroyRenderPass(mDevice, mRenderPass, nullptr);

    //Destroy all of the image views
    for (auto imageView : mSwapChainImageViews)
        vkDestroyImageView(mDevice, imageView, nullptr);

    //Destroy swap chain
    vkDestroySwapchainKHR(mDevice, mSwapChain, nullptr);

    //Destroy logical device
    vkDestroyDevice(mDevice, nullptr);

    //If debugging active, destroy messenger
    if (enableValidationLayers)
        mDebugger.DestroyDebugUtilsMessengerEXT(mInstance, mDebugger.debugMessenger, nullptr);

    //Destroy window surface
    vkDestroySurfaceKHR(mInstance, mSurface, nullptr);
    
    //Destroy instance
    vkDestroyInstance(mInstance, nullptr);
    SDL_DestroyWindow(mWindow);
    SDL_Quit();
}

void renderApp::createInstance()
{
    //Check for debugging tools
    if (enableValidationLayers && !mDebugger.checkValidationLayerSupport())
        throw std::runtime_error("Validation layers requested, but are unavailable!\n");

    //Fill struct with application info (optional)
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Rubik Rescue";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "Test Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    //Fill struct with instance info (requried)
    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    //Update createInfo struct with validation layers depending on debug or release mode
    if (enableValidationLayers)
    {
        createInfo.enabledLayerCount = static_cast<uint32_t>(mDebugger.validationLayers.size());
        createInfo.ppEnabledLayerNames = mDebugger.validationLayers.data();
    }
    else
        createInfo.enabledLayerCount = 0;

    //Get the SDL instance extensions required for Vulkan
    auto sdlExtensions = mDebugger.getRequiredExtensions();
    createInfo.ppEnabledExtensionNames = sdlExtensions.data();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(sdlExtensions.size());

    //Create additional debug messenger
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if (enableValidationLayers) 
    {
        createInfo.enabledLayerCount = static_cast<uint32_t>(mDebugger.validationLayers.size());
        createInfo.ppEnabledLayerNames = mDebugger.validationLayers.data();
        mDebugger.populateDebugMessengerCreateInfo(debugCreateInfo);
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
    }
    else 
    {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
    }

    //Create the Vulkan instance with all of the information we acquired and check if successfull
    VkResult result = vkCreateInstance(&createInfo, nullptr, &mInstance);
    if (result != VK_SUCCESS)
        throw std::runtime_error(("Failed to create Vulkan instance! Vulkan Result: %s\n", string_VkResult(result)));

    //Retrieve list of extensions supported by Vulkan
    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> extensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
    std::cout << "Available Supported Extensions:\n";
    for (const auto& extension : extensions)
        std::cout << '\t' << extension.extensionName << '\n';
}

void renderApp::pickPhysicalDevice()
{
    //Graphics card is selected and stored as VkPhysicalDevice
    mPhysicalDevice = VK_NULL_HANDLE;
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(mInstance, &deviceCount, nullptr);

    //Check for available Vulkan supported devices
    if (deviceCount == 0) 
        throw std::runtime_error("Failed to find a GPU with Vulkan support!");
    
    //If there are available graphics cards, add them to the list of devices
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(mInstance, &deviceCount, devices.data());

    //Iterate through list of available devices and query if each device is suitable.
    //If it is then choose it as the GPU
    //Use an ordered map to automatically sort candidates by increasing score
    std::multimap<int, VkPhysicalDevice> candidates;

    //Build the map of devices, querying the score of each device
    for (const auto& device : devices) 
    {
        int score = rateDeviceSuitability(device);
        candidates.insert(std::make_pair(score, device));
    }

    //Check if the best candidate is suitable at all, if not then throw an exception
    //first = score
    //second = device
    //Start at ending since multimap is sorted in ascending order
    if (candidates.rbegin()->first > 0) 
        mPhysicalDevice = candidates.rbegin()->second;
    else 
        throw std::runtime_error("Failed to find a suitable GPU!");
}

int renderApp::rateDeviceSuitability(VkPhysicalDevice device)
{

    //Query the device name, type, and supported version of Vulkan
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);

    //Query the device for optional features (texture compression, multiple viewports, etc.)
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    int score = 0;

    //Dedicated GPUs have a significant performance advantage
    if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        score += 1000;

    //Maximum possible size of textures affects graphics quality
    score += deviceProperties.limits.maxImageDimension2D;

    //Query the queue families of the device
    QueueFamilyIndices indices = findQueueFamilies(device);

    bool extensionsSupported = checkDeviceExtensionSupport(device);
    bool validSwapChain = false;
    if (extensionsSupported)
    {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
        validSwapChain = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    //Device needs to support geometry shaders and the device needs to have a queue family and the device needs to support swap chain extension
    if (!deviceFeatures.geometryShader || !indices.isComplete() || !extensionsSupported || !validSwapChain)
        return 0;

    return score;
}

QueueFamilyIndices renderApp::findQueueFamilies(VkPhysicalDevice device)
{
    QueueFamilyIndices indices;

    //Retrieve the list of queue families and their properties
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);   //This struct contains information about the type of operations that are supported
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    //We must find a queue family that supports VK_QUEUE_GRAPHICS_BIT
    //We must also find a queue family that supports window surface presentation
    int i = 0;
    for (const auto& queueFamily : queueFamilies) 
    {
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, mSurface, &presentSupport);
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            indices.graphicsFamily = i;
        if (presentSupport)
            indices.presentFamily = i;
        if (indices.isComplete())
            break;
        i++;
    }

    return indices;
}

void renderApp::createLogicalDevice()
{
    //Query the queue families for the graphics card
    QueueFamilyIndices indices = findQueueFamilies(mPhysicalDevice);

    //Create a set of all unique queue families that are necessary for required queues
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

    //Specify the details of the device queue struct
    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies)
    {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueFamilyIndex = indices.graphicsFamily.value();
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    //Specify the set of device features to use
    VkPhysicalDeviceFeatures deviceFeatures{};

    //Specify details of the device struct
    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    //Point to the vector of VkDeviceQueueCreateInfos
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(mDeviceExtensions.size()); //These last two calls enable the swap chain extension
    createInfo.ppEnabledExtensionNames = mDeviceExtensions.data();                      //

    //Check for debugging
    if (enableValidationLayers) 
    {
        createInfo.enabledLayerCount = static_cast<uint32_t>(mDebugger.validationLayers.size());
        createInfo.ppEnabledLayerNames = mDebugger.validationLayers.data();
    } 
    else
        createInfo.enabledLayerCount = 0;
    
    //If creating logical device fails, throw exception
    if (vkCreateDevice(mPhysicalDevice, &createInfo, nullptr, &mDevice) != VK_SUCCESS)
        throw std::runtime_error("Failed to create logical device!");

    //Retrieve queue handles for each queue family
    //Index is 0 because we are only creating 1 queue from this family
    vkGetDeviceQueue(mDevice, indices.graphicsFamily.value(), 0, &mGraphicsQueue);
    vkGetDeviceQueue(mDevice, indices.presentFamily.value(), 0, &mPresentQueue);
}

void renderApp::createSurface()
{
    //Create a window surface and check if it was successful
    if (SDL_Vulkan_CreateSurface(mWindow, mInstance, &mSurface) != SDL_TRUE)
        throw std::runtime_error("Failed to create window surface!");
}

bool renderApp::checkDeviceExtensionSupport(VkPhysicalDevice device)
{
    //Record the number of available extensions supported by the device
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    //Record the available extensions into a vector
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    //Use a set to represent unconfirmed required extensions.
    //This allows us to erase extensions that are required based off of the extensions that are available by the device.
    std::set<std::string> requiredExtensions(mDeviceExtensions.begin(), mDeviceExtensions.end());

    //Each enumerated extension has a name corresponding to its type. This can be used to "elimnate" the swap chain extension that we require
    for (const auto& extension : availableExtensions)
        requiredExtensions.erase(extension.extensionName);
    
    return requiredExtensions.empty();
}

SwapChainSupportDetails renderApp::querySwapChainSupport(VkPhysicalDevice device)
{
    SwapChainSupportDetails details;

    //Determine supported surface capabilities taking into account the VkPhysicalDevice and the VkSurfaceKHR
    //This is because the device and the surface are the core components of the swap chain
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, mSurface, &details.capabilities);

    //Determine device surface formats
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, mSurface, &formatCount, nullptr);
    if (formatCount != 0)
    {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, mSurface, &formatCount, details.formats.data());
    }

    //Determine available presentation modes
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, mSurface, &presentModeCount, nullptr);
    if(presentModeCount != 0)
    {
        details.presentModes.resize(formatCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, mSurface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

VkSurfaceFormatKHR renderApp::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
    for (const auto& availableFormat : availableFormats)
        //Look for format with 32 bits of of color per pixel in SRGB space
        //Also check for SRGB color space support
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            return availableFormat;

    //If neither setting is there, return the first available format
    return availableFormats[0];
}

VkPresentModeKHR renderApp::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
    //Look for triple buffering present mode which has less latency issues than standard v-sync but still avoids tearing
    for (const auto& availablePresentMode : availablePresentModes)
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
            return availablePresentMode;

    //Similar to traditional v-sync in video games
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D renderApp::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
    //If the current extent is not at the maximum value of uint32_t then return it
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
        return capabilities.currentExtent;
    else
    {
        //Retrieve and record the width and height of the SDL frame buffer
        int width, height;
        SDL_Vulkan_GetDrawableSize(mWindow, &width, &height);

        //Cast the width and height of the frame buffer to uint32_t
        VkExtent2D actualExtent = 
        {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        //Clamp the actual extent of the width and height to be within the supported bounds of the implementation
        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        //Return the new extent of the image
        return actualExtent;
    }
}

void renderApp::createSwapChain()
{
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(mPhysicalDevice);

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

    //Specify the minimum number of images in the swap chain as the minimum plus one. This is to avoid waiting for the driver to finish internal operations
    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
        imageCount = swapChainSupport.capabilities.maxImageCount;

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = mSurface;                                      //Specify the surface to tie the swap chain to
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;                                    //Specify the amount of layers each image consists of
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;        //Specify what kind of operations to use the images in the swap chain for (render directly to them using color attachment bit)
                                                                        //Use VK_IMAGE_USAGE_TRANSFER_DST_BIT and a memory operation for post processing and rendering to an image first

    //Specify how to handle swap chain images that will be used across multiple queue families
    QueueFamilyIndices indices = findQueueFamilies(mPhysicalDevice);
    uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };
    if (indices.graphicsFamily != indices.presentFamily)
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;   //Images can be used across multiple queue families without explicit ownership transfer
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; //An image is owned by one queue family at a time and ownership must be explicitly transferred. Best performance
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform; //Rotation, translation of image if supported. Current transform is no change
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;            // Specify if the alpha channel should be used for blending with other windows
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;                                             //Ignore the color of blocked/obscured pixels
    createInfo.oldSwapchain = VK_NULL_HANDLE;                                 //It is possible for a swwap chain to become invalid or unoptimized while the application is running (i.e. window resizing)

    if (vkCreateSwapchainKHR(mDevice, &createInfo, nullptr, &mSwapChain) != VK_SUCCESS)
        throw std::runtime_error("Failed to create swap chain!");

    //Retrieve swap chain images
    //Query the final number of images, resize the container, and query again to retrieve the handles
    vkGetSwapchainImagesKHR(mDevice, mSwapChain, &imageCount, nullptr);
    mSwapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(mDevice, mSwapChain, &imageCount, mSwapChainImages.data());

    //Store image format and extent as member variables for future use 
    mSwapChainImageFormat = surfaceFormat.format;
    mSwapChainExtent = extent;
}

void renderApp::createImageViews()
{
    mSwapChainImageViews.resize(mSwapChainImages.size());
    for (size_t i = 0; i < mSwapChainImages.size(); i++)
    {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = mSwapChainImages[i];

        //How the image data should be interpreted
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;    //1D, 2D, 3D textures. Cubemaps. etc.
        createInfo.format = mSwapChainImageFormat;

        //Allows for color channel swizzling. This is the default color mapping
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        //Describe the image's purpose and which part of the image to access. In this case they are used as color targets without mipmapping or multiple layers
        //Stereographics applications would require more than one layer
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(mDevice, &createInfo, nullptr, &mSwapChainImageViews[i]) != VK_SUCCESS)
            throw std::runtime_error("Failed to create image views!");
    }
}

void renderApp::createGraphicsPipeline()
{
    //Load shader files
    auto vertShaderCode = readFile("../../../../shaders/vert.spv");
    auto fragShaderCode = readFile("../../../../shaders/frag.spv");

    //Create shader modules (wrapper for shader bytecode)
    VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
    VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

    //Create shader stage for graphics pipeline for vertex shader
    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";     //The invokable function, AKA the entrypoint, which can be used to differentiate between multiple shaders combined in one shader module

    //Create shader stage for graphics pipeline for fragment shader
    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    //Array holding the shader stage info that will be referenced at pipeline creation
    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    //Describes the format of vertex data which will be passed to the vertex shader
    //Binding is the spacing between data and whether the data is per-vertex or per-instance(instancing)
    //Attributes is the type of attributes passed to the vertex shader, the binding to load them from, and at which offset
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.pVertexBindingDescriptions = nullptr; //Optional: point to an array of structs describing details for loading vertex data
    vertexInputInfo.vertexAttributeDescriptionCount = 0;
    vertexInputInfo.pVertexAttributeDescriptions = nullptr; //Optional

    //Describes what kind of geometry will be drawn (topology) and if primitive restart should be enabled
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
    inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;   //Form a triangle from every every 3 vertices without reuse
    inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

    //We are using dynamic viewports and scissor rectangles, so we only need to specify the count at pipeline creation
    //They will later be set up at draw time
    VkPipelineViewportStateCreateInfo viewportStateInfo{};
    viewportStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportStateInfo.viewportCount = 1;
    viewportStateInfo.scissorCount = 1;

    //The rasterization state takes the vertices from the vertex shader and turns them into fragments to be colored by the fragment shader
    VkPipelineRasterizationStateCreateInfo rasterizationInfo{};
    rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizationInfo.depthClampEnable = VK_FALSE;         //If true, this clamps objects outside the near and far plane instead of discarding them. Useful for depth/shadow maps
    rasterizationInfo.rasterizerDiscardEnable = VK_FALSE;  //If true, geometry never passes through rasterization. Essentially disables output to the framebuffer
    rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;  //Determines how fragments are generated for geometry. In this case fill the polygon with fragments
    rasterizationInfo.lineWidth = 1.0f;                    //Describes the thickeness of lines in terms of number of fragments
    rasterizationInfo.cullMode = VK_CULL_MODE_BACK_BIT;    //Determine the type of face culling to use. Here we cull the back face
    rasterizationInfo.frontFace = VK_FRONT_FACE_CLOCKWISE; //Specifies the vertex order for faces to be considered front facing
    rasterizationInfo.depthBiasEnable = VK_FALSE;          //Sometimes used for shadow mapping
    rasterizationInfo.depthBiasConstantFactor = 0.0f;      //Optional
    rasterizationInfo.depthBiasClamp = 0.0f;               //Optional
    rasterizationInfo.depthBiasSlopeFactor = 0.0f;         //Optional

    //Configure multisampling. 
    VkPipelineMultisampleStateCreateInfo multisamplingInfo{};
    multisamplingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisamplingInfo.sampleShadingEnable = VK_FALSE;
    multisamplingInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisamplingInfo.minSampleShading = 1.0f; // Optional
    multisamplingInfo.pSampleMask = nullptr; // Optional
    multisamplingInfo.alphaToCoverageEnable = VK_FALSE; // Optional
    multisamplingInfo.alphaToOneEnable = VK_FALSE; // Optional

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

    VkPipelineColorBlendStateCreateInfo colorBlendingInfo{};
    colorBlendingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendingInfo.logicOpEnable = VK_FALSE;
    colorBlendingInfo.logicOp = VK_LOGIC_OP_COPY; // Optional
    colorBlendingInfo.attachmentCount = 1;
    colorBlendingInfo.pAttachments = &colorBlendAttachment;
    colorBlendingInfo.blendConstants[0] = 0.0f; // Optional
    colorBlendingInfo.blendConstants[1] = 0.0f; // Optional
    colorBlendingInfo.blendConstants[2] = 0.0f; // Optional
    colorBlendingInfo.blendConstants[3] = 0.0f; // Optional

    //Dynamic states specify the pipeline states that are not baked into the pipeline and can be changed at draw time without recreating the pipeline
    std::vector<VkDynamicState> dynamicStates = 
    {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    //This allows for a more flexible set up
    VkPipelineDynamicStateCreateInfo dynamicStateInfo{};
    dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicStateInfo.pDynamicStates = dynamicStates.data();

    //Used to specify uniform values for shaders
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0; // Optional
    pipelineLayoutInfo.pSetLayouts = nullptr; // Optional
    pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
    pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

    if (vkCreatePipelineLayout(mDevice, &pipelineLayoutInfo, nullptr, &mPipelineLayout) != VK_SUCCESS)
        throw std::runtime_error("Failed to create pipeline layout!");

    //Comnbine all of the structures and pipeline states to create a graphics pipeline using a create info struct
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
    pipelineInfo.pViewportState = &viewportStateInfo;
    pipelineInfo.pRasterizationState = &rasterizationInfo;
    pipelineInfo.pMultisampleState = &multisamplingInfo;
    pipelineInfo.pDepthStencilState = nullptr; // Optional: for depth stencil buffers only
    pipelineInfo.pColorBlendState = &colorBlendingInfo;
    pipelineInfo.pDynamicState = &dynamicStateInfo;
    pipelineInfo.layout = mPipelineLayout;
    pipelineInfo.renderPass = mRenderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional: used to derive an existing pipeline
    pipelineInfo.basePipelineIndex = -1; // Optional

    if (vkCreateGraphicsPipelines(mDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &mGraphicsPipeline) != VK_SUCCESS)
        throw std::runtime_error("failed to create graphics pipeline!");

    //Destroy shader modules
    vkDestroyShaderModule(mDevice, vertShaderModule, nullptr);
    vkDestroyShaderModule(mDevice, fragShaderModule, nullptr);
}

std::vector<char> renderApp::readFile(const std::string& filename)
{
    //Start reading the file At The End, in binary
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file.is_open())
        throw std::runtime_error("Failed to open file!");

    //We start at the end to determine the size of the file by the read position. Then we allocate a buffer
    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);

    //Go back to beginning of file and read it into the buffer
    file.seekg(0);
    file.read(buffer.data(), fileSize);

    //Close file, return bytes
    file.close();
    return buffer;
}

VkShaderModule renderApp::createShaderModule(const std::vector<char>& code)
{
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    //Bytecode pointer is a uint32_t, but we have a char pointer so we must cast it to uint32_t
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(mDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
        throw std::runtime_error("Failed to create shader module!");

    return shaderModule;
}

void renderApp::createRenderPass()
{
    VkAttachmentDescription colorAttachment{};                  //Only use one color buffer attachment represented by an image from the swap chain
    colorAttachment.format = mSwapChainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;            //Change for multisampling

    //This applies to color buffer operations before and after rendering
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;       //Clear the attachment data to a constant at the start. Clears framebuffer to black before drawing a new fram
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;     //Rendered contents are stored in memory to be read later. This allows us to draw the triangle to the screen

    //This applies to stencil data, but we are not using a stencil buffer so we do not care
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;      //We do not care about the previous image's layout since we will clear it anyways
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;  //We want the image to be ready for presentation after rendering using the swap chain

    //Reference to the color buffer attachment
    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;  //Specify which attachment to reference in the attachment descriptions array by its index
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;   //Specify which layout the attachment should have during a subpass that uses it. Optimal gives the best performance

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    //Specify reference to color attachment
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    //Set up subpass dependency
    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(mDevice, &renderPassInfo, nullptr, &mRenderPass) != VK_SUCCESS)
        throw std::runtime_error("Failed to create a render pass!");
}

void renderApp::createFramebuffers()
{
    //Resize to hold all the framebuffers
    mSwapChainFramebuffers.resize(mSwapChainImageViews.size());

    //Create framebuffers by looping through image views
    for (size_t i = 0; i < mSwapChainImageViews.size(); i++)
    {
        VkImageView attachments[] = { mSwapChainImageViews[i] };
        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = mRenderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = mSwapChainExtent.width;
        framebufferInfo.height = mSwapChainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(mDevice, &framebufferInfo, nullptr, &mSwapChainFramebuffers[i]) != VK_SUCCESS)
            throw std::runtime_error("Failed to create framebuffer!");
    }
}

void renderApp::createCommandPool()
{
    QueueFamilyIndices queueFamilyIndices = findQueueFamilies(mPhysicalDevice);
    VkCommandPoolCreateInfo commandPoolCreateInfo{};
    commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;      //Allow command buffers to be rerecorded individually, without this flag they all have to be reset together. 
                                                                                        //We need to record a command buffer every frame which means we want to reswet it and rerecord over it.
    commandPoolCreateInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value(); //Record commands for drawing => choose graphics family

    if (vkCreateCommandPool(mDevice, &commandPoolCreateInfo, nullptr, &mCommandPool))
        throw std::runtime_error("Failed to create command pool!");
}

void renderApp::createCommandBuffer()
{
    VkCommandBufferAllocateInfo commandBufferAllocateInfo{};
    commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocateInfo.commandPool = mCommandPool;
    commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;                  //Allows command buffer to be directly submitted to a queue, but not called from other command buffers.
    commandBufferAllocateInfo.commandBufferCount = 1;
    
    if (vkAllocateCommandBuffers(mDevice, &commandBufferAllocateInfo, &mCommandBuffer) != VK_SUCCESS)
        throw std::runtime_error("Failed to allocate command buffers!");
}

void renderApp::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
{
    VkCommandBufferBeginInfo commandBufferBeginInfo{};
    commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    commandBufferBeginInfo.flags = 0;                   //Optional: Specifies how we use the command buffer
    commandBufferBeginInfo.pInheritanceInfo = nullptr;  //Optional: Only useful for secondary command buffers. It specifies which state to inherit from the calling primary command buffers.

    if (vkBeginCommandBuffer(mCommandBuffer, &commandBufferBeginInfo) != VK_SUCCESS)    //If the command buffer was recorded already, then a new call to vkBeginCommandBuffer resets it. It does NOT append commands to a buffer.
        throw std::runtime_error("Failed to begin recording command buffers!");

    VkRenderPassBeginInfo renderPassBeginInfo{};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderPass = mRenderPass;                           //The render pass
    renderPassBeginInfo.framebuffer = mSwapChainFramebuffers[imageIndex];   //The attachments to bind to the render pass

    //We have a framebuffer for each swap chain image where each is specified as a color attachment
    //We need to bind the framebuffer for the swap chain image we want to draw to by uysing the imageIndex
    renderPassBeginInfo.renderArea.offset = { 0, 0 };
    renderPassBeginInfo.renderArea.extent = mSwapChainExtent;   //These values define where shader loads and stores take place. Pixels outside are undefined. Should match the size of the attachments for best performance.

    VkClearValue clearColor = {{{ 0.0f, 0.0f, 0.0f, 1.0f }}};
    renderPassBeginInfo.clearValueCount = 1;
    renderPassBeginInfo.pClearValues = &clearColor;     //These parameters define the clear values VK_ATTACHMENT_LOAD_OP_CLEAR which was used as load operation for color attachment

    //All functions that record commands (vkCmd) return void, so there is no error handling for these
    vkCmdBeginRenderPass(mCommandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE); //This begins the render pass and specifies how the drawing commands within the render pass are provided. 
                                                                                            //Render pass commands are embedded in the primary command buffer with no secondary command buffers being executed.

    //This tells Vulkan which operations to execute in the graphics pipeline and which attachment to use in the fragment shader
    vkCmdBindPipeline(mCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mGraphicsPipeline); //Specify if the pipeline is compute or graphics

    //Set the viewport for the command buffer
    VkViewport viewport{};
    viewport.x, viewport.y = 0.0f;
    viewport.width = static_cast<float>(mSwapChainExtent.width);
    viewport.height = static_cast<float>(mSwapChainExtent.height);    
    viewport.minDepth = 0.0f,
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(mCommandBuffer, 0, 1, &viewport);

    //Set the scissor state for the command buffer
    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = mSwapChainExtent;
    vkCmdSetScissor(mCommandBuffer, 0, 1, &scissor);

    //Issue the draw command (commandBuffer, vertices, instances, first vertex, first instance)
    vkCmdDraw(mCommandBuffer, 3, 1, 0, 0);

    //End the render pass
    vkCmdEndRenderPass(mCommandBuffer);

    if (vkEndCommandBuffer(mCommandBuffer) != VK_SUCCESS)
        throw std::runtime_error("Failed to record command buffer!");
}

void renderApp::drawFrame()
{
    //Wait for host to signal all fences, then reset the fence to an unsignaled state
    vkWaitForFences(mDevice, 1, &mInFlightFence, VK_TRUE, UINT64_MAX);
    vkResetFences(mDevice, 1, &mInFlightFence);

    //Acquire image from the swapchain
    uint32_t imageIndex;
    vkAcquireNextImageKHR(mDevice, mSwapChain, UINT64_MAX, mSwapchainSemaphore, VK_NULL_HANDLE, &imageIndex);    //When image is fetched, signal the swapchain semaphore

    //Reset the command buffer to allow for recording, then record the command buffer
    vkResetCommandBuffer(mCommandBuffer, 0);
    recordCommandBuffer(mCommandBuffer, imageIndex);

    //Prepare to submit the command buffer
    VkSubmitInfo commandBufferSubmitInfo{};
    commandBufferSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    VkSemaphore waitSemaphores[] = { mSwapchainSemaphore };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };  //Stage of the pipeline that write to color attachment
    commandBufferSubmitInfo.waitSemaphoreCount = 1;
    commandBufferSubmitInfo.pWaitSemaphores = waitSemaphores;   //Specify which semaphores to wait on
    commandBufferSubmitInfo.pWaitDstStageMask = waitStages;     //Specify which stage of the graphics pipeline to wait (in this case the stage that writes colors to the image)
    commandBufferSubmitInfo.commandBufferCount = 1;
    commandBufferSubmitInfo.pCommandBuffers = &mCommandBuffer;  //Specify which command buffer to submit

    VkSemaphore signalSemaphores[] = { mRenderingSemaphore };
    commandBufferSubmitInfo.signalSemaphoreCount = 1;
    commandBufferSubmitInfo.pSignalSemaphores = signalSemaphores; //Specify which semaphore to signal once the command buffer finishes executing

    //Submit the command buffer to the graphcis queue, specifying the fence to signal in order to tell if the command buffer can be reused
    if (vkQueueSubmit(mGraphicsQueue, 1, &commandBufferSubmitInfo, mInFlightFence) != VK_SUCCESS)
        throw std::runtime_error("Failed to submit draw command buffer!");

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores; //Specify which semaphore to wait on before presentation

    VkSwapchainKHR swapChains[] = { mSwapChain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr; //Optional: allows for speficication of an array of VK_RESULT values to check if each swapchain presentation was successful or not. More useful when using more than one swapchain

    vkQueuePresentKHR(mPresentQueue, &presentInfo); //Submit request to present image to swap chain
}

void renderApp::createSyncObjects()
{
    VkSemaphoreCreateInfo semaphoreCreateInfo{};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceCreateInfo{};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;   //Create fence in signaled state so that the first frame returns immediately

    if (vkCreateSemaphore(mDevice, &semaphoreCreateInfo, nullptr, &mSwapchainSemaphore) != VK_SUCCESS ||
        vkCreateSemaphore(mDevice, &semaphoreCreateInfo, nullptr, &mRenderingSemaphore) != VK_SUCCESS ||
        vkCreateFence(mDevice, &fenceCreateInfo, nullptr, &mInFlightFence) != VK_SUCCESS)
        throw std::runtime_error("Failed to create synchronization objects!");
}


void renderApp::run()
{
    initWindow();
    initVulkan();
    loop();
    clean();
}
