#include <iostream>
#include <SDL.h>
#include <SDL_vulkan.h>
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

#include "renderApp.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#undef main

const uint32_t SCREEN_WIDTH = 1080;
const uint32_t SCREEN_HEIGHT = 720;

int main()
{
    renderApp app;
    try 
    {
        app.run();
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << "\n";
        return EXIT_FAILURE;
    }

    SDL_Window* window = NULL;
    SDL_Surface* surface = NULL;

    //If you cannot intialize SDL2, exit with error code    
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        std::cout << "Could not initialize SDL! SDL_Error: " << SDL_GetError() << "\n";
        return EXIT_FAILURE;
    }

    window = SDL_CreateWindow("Vulkan Test", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_VULKAN);

    //If you cannot create the window, exit with error code    
    if (window == NULL)
    {
        std::cout << "Window could not be created! SDL_Error: " << SDL_GetError() << "\n";
        return EXIT_FAILURE;
    }

    SDL_SetWindowResizable(window, SDL_FALSE);

    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    std::cout << extensionCount << " extensions supported\n";

    SDL_Event event;
    bool running = true;

    //Main loop
    while (running)
    {
        while (SDL_PollEvent(&event))
            if (event.type == SDL_QUIT)
                running= false;
    }

    SDL_DestroyWindow(window);
    SDL_Quit();
    return EXIT_SUCCESS;
}
