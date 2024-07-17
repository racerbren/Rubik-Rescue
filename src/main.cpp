#include <iostream>
#include <SDL.h>

#undef main

const uint32_t width = 1080;
const uint32_t height = 720;

int main()
{
    SDL_Window* window = NULL;
    SDL_Surface* surface = NULL;

    if (SDL_Init(SDL_INIT_VIDEO) < 0)
        std::cout << "Could not initialize SDL! SDL_Error: " << SDL_GetError() << "\n";
    else
    {
        window = SDL_CreateWindow("SDL Test", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_SHOWN);
        if (window == NULL)
            std::cout << "Window could not be created! SDL_Error: " << SDL_GetError() << "\n";
        else
        {
            surface = SDL_GetWindowSurface(window);
            SDL_FillRect(surface, NULL, SDL_MapRGB(surface->format, 0x00, 0xFF, 0xFF));
            SDL_UpdateWindowSurface(window);
            SDL_Event e;
            bool running = true;
            while (running)
            {
                while (SDL_PollEvent(&e))
                    if (e.type == SDL_QUIT)
                        running= false;
            }
        }
    }
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
