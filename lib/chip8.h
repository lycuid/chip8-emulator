/*
 * References:
 *    - https://en.wikipedia.org/wiki/CHIP-8
 *    - http://devernay.free.fr/hacks/chip8/C8TECH10.HTM
 */
#ifndef __CHIP8_H__
#define __CHIP8_H__

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#define MAX_ROM_SIZE (0xfff - 0x1ff)
#define WIDTH        64
#define HEIGHT       32

typedef uint8_t byte;

typedef struct Chip8 {
    byte memory[4096];  // 4k ram; byte[4096].
    uint16_t stack[16]; // stack; u16[16].
    byte V[16];         // registers byte[16].
    uint16_t I;         // Index pointer; [0x000..0xfff].
    uint16_t pc;        // Program counter; [0x000..=0xfff].
    size_t sp;          // Stack pointer.
    bool keypad[16];    // key pressed; byte[16].
    byte dt;            // delay timer.
    byte st;            // sound timer.
    byte display[WIDTH * HEIGHT];
} Chip8;

void chip8_init(Chip8 *);
void chip8_load_rom(Chip8 *, const char[MAX_ROM_SIZE]);
void chip8_update_keypad(Chip8 *, char, bool);
void chip8_emulate_cycle(Chip8 *);

#endif
