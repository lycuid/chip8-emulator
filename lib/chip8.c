#include "chip8.h"
#include <string.h>
#include <time.h>

static const byte fontset[] = {
    0xf0, 0x90, 0x90, 0x90, 0xf0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xf0, 0x10, 0xf0, 0x80, 0xf0, // 2
    0xf0, 0x10, 0xf0, 0x10, 0xf0, // 3
    0x90, 0x90, 0xf0, 0x10, 0x10, // 4
    0xf0, 0x80, 0xf0, 0x10, 0xf0, // 5
    0xf0, 0x80, 0xf0, 0x90, 0xf0, // 6
    0xf0, 0x10, 0x20, 0x40, 0x40, // 7
    0xf0, 0x90, 0xf0, 0x90, 0xf0, // 8
    0xf0, 0x90, 0xf0, 0x10, 0xf0, // 9
    0xf0, 0x90, 0xf0, 0x90, 0x90, // a
    0xe0, 0x90, 0xe0, 0x90, 0xe0, // b
    0xf0, 0x80, 0x80, 0x80, 0xf0, // c
    0xe0, 0x90, 0x90, 0x90, 0xe0, // d
    0xf0, 0x80, 0xf0, 0x80, 0xf0, // e
    0xf0, 0x80, 0xf0, 0x80, 0x80, // f
};

static inline void beep(void) { (void)0; }
static inline int random_byte(void) { return rand() % 255 + 1; }
static inline void step(Chip8 *chip8) { chip8->pc += 2; }
static inline void execute_opcode(Chip8 *, uint16_t);

static inline void clear_display(Chip8 *chip8)
{
    memset(chip8->display, 0, WIDTH * HEIGHT * sizeof(chip8->display[0]));
}

static inline void chip8_clear(Chip8 *chip8)
{
    memset(chip8->stack, 0, sizeof(chip8->stack));
    memset(chip8->V, 0, sizeof(chip8->V));
    memset(chip8->keypad, false, sizeof(chip8->keypad));
    clear_display(chip8);
    chip8->I  = 0;
    chip8->pc = 0x200;
    chip8->sp = 0;
    chip8->dt = 0;
    chip8->st = 0;
}

void chip8_init(Chip8 *chip8)
{
    srand(time(NULL));

    chip8_clear(chip8);
    // memory[0x000..0x1ff] = fontset;
    memcpy(chip8->memory, fontset, sizeof(fontset) / sizeof(fontset[0]));
}

void chip8_load_rom(Chip8 *chip8, const char rom[MAX_ROM_SIZE])
{
    // memory[0x200..0xfff] = rom;
    memcpy(chip8->memory + 0x200, rom, MAX_ROM_SIZE);
    chip8_clear(chip8);
}

void chip8_update_keypad(Chip8 *chip8, char keycode, bool state)
{
    switch (keycode) {
    case 'X': chip8->keypad[0] = state; break;
    case '1': chip8->keypad[1] = state; break;
    case '2': chip8->keypad[2] = state; break;
    case '3': chip8->keypad[3] = state; break;
    case 'Q': chip8->keypad[4] = state; break;
    case 'W': chip8->keypad[5] = state; break;
    case 'E': chip8->keypad[6] = state; break;
    case 'A': chip8->keypad[7] = state; break;
    case 'S': chip8->keypad[8] = state; break;
    case 'D': chip8->keypad[9] = state; break;
    case 'Z': chip8->keypad[10] = state; break;
    case 'C': chip8->keypad[11] = state; break;
    case '4': chip8->keypad[12] = state; break;
    case 'R': chip8->keypad[13] = state; break;
    case 'F': chip8->keypad[14] = state; break;
    case 'V': chip8->keypad[15] = state; break;
    default: break;
    };
}

void chip8_emulate_cycle(Chip8 *chip8)
{
    execute_opcode(chip8, chip8->memory[chip8->pc] << 8 |
                              chip8->memory[chip8->pc + 1]);

    // update timer.
    chip8->dt -= chip8->dt > 0;
    if (chip8->st > 0) {
        if (chip8->st == 1)
            beep();
        --chip8->st;
    }
}

static inline void execute_opcode(Chip8 *chip8, uint16_t opcode)
{
    size_t x     = (opcode & 0x0f00) >> 8;
    size_t y     = (opcode & 0x00f0) >> 4;
    byte kk      = (opcode & 0x00ff);
    size_t n     = opcode & 0x000f;
    uint16_t nnn = opcode & 0x0fff;

    switch ((opcode & 0xf000) >> 12) {
    case 0x0: {
        switch (n) {
        // `00E0` -> CLS
        case 0x0: {
            clear_display(chip8);
        } break;

        // `00EE` -> RET
        case 0xe: {
            chip8->pc = chip8->stack[--chip8->sp];
            return;
        } break;

        default: exit(2);
        }
    } break;

    // `1nnn` -> JP addr
    //    - set program counter to `nnn`.
    case 0x1: {
        chip8->pc = nnn;
        return;
    } break;

    // `2nnn` -> CALL addr
    //    - current program counter goes on top the stack.
    //    - increment stack pointer.
    //    - set program counter to `nnn`.
    case 0x2: {
        chip8->stack[chip8->sp] = chip8->pc + 2;
        chip8->sp++;
        chip8->pc = nnn;
        return;
    } break;

    // `3xkk` -> SE Vx, byte
    //    - skip next instruction if `Vx == kk`.
    case 0x3: {
        if (chip8->V[x] == kk)
            step(chip8);
    } break;

    // `4xkk` -> SNE Vx, byte
    //    - skip next instruction if `Vx != kk`.
    case 0x4: {
        if (chip8->V[x] != kk)
            step(chip8);
    } break;

    // `5xy0` -> SE Vx, Vy
    //    - skip next instruction if `Vx == Vy`.
    case 0x5: {
        if (chip8->V[x] == chip8->V[y])
            step(chip8);
    } break;

    // `6xkk` -> LD Vx, byte
    //    - put value `kk` into register `Vx`.
    case 0x6: chip8->V[x] = kk; break;

    // `7xkk` -> ADD Vx, byte
    //    - set `Vx = Vx + kk`.
    case 0x7: chip8->V[x] = chip8->V[x] + kk; break;

    case 0x8: {
        switch (n) {
        // `8xy0` -> LD Vx, Vy
        //    - set `Vx = Vy`.
        case 0x0: chip8->V[x] = chip8->V[y]; break;

        // `8xy1` -> OR Vx, Vy
        //    - set `Vx = Vx | Vy`.
        case 0x1: chip8->V[x] |= chip8->V[y]; break;

        // `8xy2` -> AND Vx, Vy
        //    - set `Vx = Vx & Vy`.
        case 0x2: chip8->V[x] &= chip8->V[y]; break;

        // `8xy3` -> XOR Vx, Vy
        //    - set `Vx = Vx ^ Vy`.
        case 0x3: chip8->V[x] ^= chip8->V[y]; break;

        // `8xy4` -> ADD Vx, Vy
        //    - calculate `Vx + Vy`
        //    - set VF to 1, if `Vx + Vy` is more than 8 bits, else 0.
        //    - store the lower 8 bits of `Vx + Vy` into `Vx`.
        case 0x4: {
            uint16_t sum  = chip8->V[x] + chip8->V[y];
            chip8->V[0xf] = sum > 0xff;
            chip8->V[x]   = sum & 0xff;
        } break;

        // `8xy5` -> SUB Vx, Vy
        //    - set `Vx = Vx - Vy`.
        //    - set VF to 1, if `Vx > Vy`, else 0.
        case 0x5: {
            chip8->V[0xf] = chip8->V[x] > chip8->V[y];
            chip8->V[x] -= chip8->V[y];
        } break;

        // `8xy6` -> SHR Vx
        //    - set `VF` to least significant bit of `Vx`
        //    - divide `Vx` by 2.
        case 0x6: {
            chip8->V[0xf] = chip8->V[x] & 1;
            chip8->V[x] >>= 1;
        } break;

        // `8xy7` -> SUBN Vx, Vy
        //    - set `Vx = Vy - Vx`.
        //    - set VF to 1, if `Vy > Vx`, else 0.
        case 0x7: {
            chip8->V[0xf] = (chip8->V[y] > chip8->V[x]);
            chip8->V[x]   = chip8->V[y] - chip8->V[x];
        } break;

        // `8xyE` -> SHL Vx, Vy
        //    - set `VF` to most significant bit of `Vx`.
        //    - multiply `Vx` by 2
        case 0xe: {
            chip8->V[0xf] = chip8->V[x] >> 7;
            chip8->V[x] <<= 1;
        } break;

        default: exit(2);
        }
    } break;

    // `9xy0` -> SNE Vx, Vy
    //    - skip next instruction if `Vx != Vy`.
    case 0x9: {
        if (chip8->V[x] != chip8->V[y])
            step(chip8);
    } break;

    // `Annn` -> LD I, addr
    //    - set `I` to `nnn`.
    case 0xa: chip8->I = nnn; break;

    // `Bnnn` -> JP V0, addr
    //    - Jump to location `nnn + V0`.
    case 0xb: {
        chip8->pc = nnn + chip8->V[0];
        return;
    } break;

    // `Cxkk` - RND Vx, byte
    //    - set `Vx` = rand AND kk.
    case 0xc: chip8->V[x] = random_byte() & kk; break;

    // `Dxyn` - DRW Vx, Vy, nibble
    // Display n-byte sprite starting at memory location I at (Vx, Vy), set VF =
    // collision.
    //
    // The interpreter reads n bytes from memory, starting at the address
    // stored in I. These bytes are then displayed as sprites on screen
    // at coordinates (Vx, Vy). Sprites are XORed onto the existing screen.
    // If this causes any pixels to be erased, VF is set to 1, otherwise
    // it is set to 0. If the sprite is positioned so part of it is outside
    // the coordinates of the display, it wraps around to the opposite side
    // of the screen.
    case 0xd: {
        byte Vx       = chip8->V[x];
        byte Vy       = chip8->V[y];
        chip8->V[0xf] = 0;

        for (size_t row = 0; row < n; ++row) {
            byte pixel = chip8->memory[chip8->I + row];
            byte y     = (Vy + row) % HEIGHT;
            for (size_t col = 0; col < 8; ++col) {
                if (pixel & (0x80 >> col)) {
                    byte x = (Vx + col) % WIDTH;
                    if (chip8->display[y * WIDTH + x])
                        chip8->V[0xf] = 1; // collision.
                    chip8->display[y * WIDTH + x] ^= 1;
                }
            }
        }
    } break;

    // `Exkk`.
    case 0xe: {
        switch (kk) {
        // `Ex9E`
        // skip next instruction if key with the value of Vx is pressed.
        // Checks the keypad, and if the key corresponding to the value
        // of Vx is currently in the down position, PC is increased by 2.
        case 0x9e: {
            if (chip8->keypad[chip8->V[x]])
                step(chip8);
        } break;
        // `ExA1` - SKNP Vx
        //    - skip next instruction if key with the value of Vx is not
        //    pressed.
        case 0xa1: {
            if (!chip8->keypad[chip8->V[x]])
                step(chip8);
        } break;
        default: exit(2);
        }
    } break;

    // `Fxkk`
    case 0xf: {
        switch (kk) {
        // `Fx07` -> LD Vx, DT
        //    - set `Vx` to `DT` value.
        case 0x07: chip8->V[x] = chip8->dt; break;

        // `Fx0A` - LD Vx, K
        //    - wait for a key press, store the value of the key in `Vx`.
        case 0x0a: {
        } break;

        // `Fx15` -> LD DT, Vx
        //    - set `DT` to `Vx` value.
        case 0x15: chip8->dt = chip8->V[x]; break;

        // `Fx18` - LD ST, Vx
        //    - Set `ST` to `Vx` value.
        case 0x18: chip8->st = chip8->V[x]; break;

        // `Fx1E` - ADD I, Vx
        //    - set `I = I + Vx`.
        case 0x1e: {
            chip8->I += chip8->V[x];
            chip8->V[0xf] = chip8->I > 0x0F00;
        } break;

        // `Fx29` - LD F, Vx
        //    - set `I` to location of sprite for digit `Vx`.
        case 0x29: chip8->I = chip8->V[x] * 5; break;

        // `Fx33` - LD B, Vx
        //    - places the hundreds digit of `Vx` at `memory[I]`.
        //    - places the tens digit of `Vx` at `memory[I + 1]`.
        //    - places the digits digit of `Vx` at `memory[I + 2]`.
        case 0x33: {
            chip8->memory[chip8->I]     = chip8->V[x] / 100;
            chip8->memory[chip8->I + 1] = (chip8->V[x] % 100) / 10;
            chip8->memory[chip8->I + 2] = chip8->V[x] % 10;
        } break;

        // `Fx55` -> LD [I], Vx
        //    - store registers V0 through Vx in memory starting at location I.
        case 0x55: memcpy(chip8->memory + chip8->I, chip8->V, x); break;

        // `Fx65` -> LD Vx, [I]
        //    - read registers V0 through Vx from memory starting at location I.
        case 0x65: memcpy(chip8->V, chip8->memory + chip8->I, x); break;

        default: exit(2);
        }
    } break;

    default: exit(2);
    }
    step(chip8);
}
