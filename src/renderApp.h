#pragma once
#include <iostream>
#include <SDL.h>
#include <SDL_vulkan.h>
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

const uint32_t SCREEN_WIDTH = 1080;
const uint32_t SCREEN_HEIGHT = 720;

class renderApp 
{
private:
    SDL_Window* window = NULL;

    void initWindow();
    void initVulkan();
    void loop();
    void clean();
public:
    void run();
};