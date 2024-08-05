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
    pickPhysicalDevice();
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
    if (enableValidationLayers)
        mDebugger.DestroyDebugUtilsMessengerEXT(mInstance, mDebugger.debugMessenger, nullptr);
    
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
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
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
        physicalDevice = candidates.rbegin()->second;
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
    if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
        score += 1000;
    }

    //Maximum possible size of textures affects graphics quality
    score += deviceProperties.limits.maxImageDimension2D;

    //Device needs to support geometry shaders
    if (!deviceFeatures.geometryShader)
        return 0;

    return score;
}

void renderApp::run()
{
    initWindow();
    initVulkan();
    loop();
    clean();
}
