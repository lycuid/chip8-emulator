#include <SDL2/SDL.h>
#include <chip8.h>
#include <fcntl.h>
#include <unistd.h>

#define SCALE 25

int main(int argc, char const *argv[])
{
    Chip8 chip8;
    char buffer[MAX_ROM_SIZE] = {0};
    {
        int fd = open(argc > 1 ? argv[1] : "rom/PONG2", O_RDONLY);
        read(fd, buffer, MAX_ROM_SIZE);
    }
    chip8_init(&chip8);
    chip8_load_rom(&chip8, buffer);

    SDL_Init(SDL_INIT_EVERYTHING);

    SDL_Window *window = SDL_CreateWindow(
        "CHIP-8 EMULATOR", 100, 100, 64 * SCALE, 32 * SCALE, SDL_WINDOW_OPENGL);
    SDL_Renderer *renderer =
        SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    SDL_Event e;
    for (bool running = true; running;) {
        while (SDL_PollEvent(&e)) {
            switch (e.type) {
            case SDL_QUIT: {
                running = false;
            } break;
            case SDL_KEYDOWN: // fallthrough
            case SDL_KEYUP: {
                char code = 0;
                switch (e.key.keysym.sym) {
                case SDLK_1: // fallthrough
                case SDLK_2: // fallthrough
                case SDLK_3: // fallthrough
                case SDLK_4: code = e.key.keysym.sym; break;
                case SDLK_q: // fallthrough
                case SDLK_w: // fallthrough
                case SDLK_e: // fallthrough
                case SDLK_r: // fallthrough
                case SDLK_a: // fallthrough
                case SDLK_s: // fallthrough
                case SDLK_d: // fallthrough
                case SDLK_f: // fallthrough
                case SDLK_z: // fallthrough
                case SDLK_x: // fallthrough
                case SDLK_c: // fallthrough
                case SDLK_v: code = e.key.keysym.sym - 32; break;
                }
                if (code)
                    chip8_update_keypad(&chip8, code, e.type == SDL_KEYDOWN);
            } break;
            }
        }
        {
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
            SDL_RenderClear(renderer);
        }
        chip8_emulate_cycle(&chip8);
        {
            SDL_SetRenderDrawColor(renderer, 0xff, 0xff, 0xff, 0);
            for (size_t i = 0; i < WIDTH * HEIGHT; ++i) {
                if (chip8.display[i]) {
                    SDL_RenderFillRect(renderer, &(SDL_Rect){
                                                     .x = (i % WIDTH) * SCALE,
                                                     .y = (i / WIDTH) * SCALE,
                                                     .w = SCALE,
                                                     .h = SCALE,
                                                 });
                }
            }
            SDL_RenderPresent(renderer);
        }
        SDL_Delay(1);
    }

    return 0;
}
