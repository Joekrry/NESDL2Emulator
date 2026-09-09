#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <stdbool.h>
#include <stdio.h>

#include "nes/types.h"

#define NES_SCREEN_WIDTH  256
#define NES_SCREEN_HEIGHT 240
#define NES_WINDOW_SCALE  3

int main(int argc, char **argv) {
    SDL_SetMainReady();

    const char *rom_path = (argc > 1) ? argv[1] : NULL;
    if (rom_path != NULL) {
        printf("ROM path: %s\n", rom_path);
    } else {
        printf("Usage: %s <rom_path>\n", argv[0]);
    }

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow(
        "nesemu",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        NES_SCREEN_WIDTH * NES_WINDOW_SCALE,
        NES_SCREEN_HEIGHT * NES_WINDOW_SCALE,
        SDL_WINDOW_SHOWN
    );

    if (window == NULL) {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    bool running = true;
    SDL_Event event;

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_KEYDOWN &&
                       event.key.keysym.sym == SDLK_ESCAPE) {
                running = false;
            }
        }
    }

    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
