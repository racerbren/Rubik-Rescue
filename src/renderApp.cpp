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
    }
}

void renderApp::clean()
{
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

void renderApp::run()
{
    initWindow();
    initVulkan();
    loop();
    clean();
}
