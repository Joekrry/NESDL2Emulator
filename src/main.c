#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <stdbool.h>
#include <stdio.h>

#include "nes/cart.h"
#include "nes/types.h"

#define NES_SCREEN_WIDTH  256
#define NES_SCREEN_HEIGHT 240
#define NES_WINDOW_SCALE  3

int main(int argc, char **argv) {
    SDL_SetMainReady();

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <rom_path>\n", argv[0]);
        return 1;
    }

    const char *rom_path = argv[1];
    nes_cart cart;
    const nes_cart_result load_result = nes_cart_load_file(&cart, rom_path);
    if (load_result != NES_CART_OK) {
        fprintf(stderr, "Failed to load '%s': %s\n", rom_path,
                nes_cart_result_str(load_result));
        return 1;
    }

    printf("Loaded %s\n", rom_path);
    printf("  format:    %s\n", cart.is_nes2 ? "NES 2.0" : "iNES");
    printf("  mapper:    %u (NROM)\n", (unsigned)cart.mapper);
    printf("  PRG-ROM:   %u KiB\n", (unsigned)(cart.prg_size / 1024u));
    printf("  CHR-%s:   %u KiB\n", cart.chr_is_ram ? "RAM" : "ROM",
           (unsigned)(cart.chr_size / 1024u));
    printf("  mirroring: %s\n", nes_mirroring_str(cart.mirroring));
    printf("  battery:   %s\n", cart.has_battery ? "yes" : "no");

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        nes_cart_unload(&cart);
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
        nes_cart_unload(&cart);
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
    nes_cart_unload(&cart);

    return 0;
}
