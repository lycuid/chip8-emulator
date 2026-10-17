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
static inline int random_byte(void) { return rand() & 0xff; }
static inline void execute_opcode(Chip8 *, uint16_t);

static inline void clear_display(Chip8 *chip8)
{
    memset(chip8->display, 0, WIDTH * HEIGHT * sizeof(chip8->display[0]));
}

static inline void chip8_clear(Chip8 *chip8)
{
    memset(chip8->cpu.stack, 0, sizeof(chip8->cpu.stack));
    memset(chip8->cpu.V, 0, sizeof(chip8->cpu.V));
    memset(chip8->keypad, false, sizeof(chip8->keypad));
    clear_display(chip8);
    chip8->cpu.I  = 0;
    chip8->cpu.pc = 0x200;
    chip8->cpu.sp = 0;
    chip8->cpu.dt = 0;
    chip8->cpu.st = 0;
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
    execute_opcode(chip8, chip8->memory[chip8->cpu.pc] << 8 |
                              chip8->memory[chip8->cpu.pc + 1]);

    // update timer.
    chip8->cpu.dt -= chip8->cpu.dt > 0;
    if (chip8->cpu.st > 0) {
        if (chip8->cpu.st == 1)
            beep();
        --chip8->cpu.st;
    }
}

#define step(cpu) (cpu)->pc += 2

static inline void execute_opcode(Chip8 *chip8, uint16_t opcode)
{
    Cpu *cpu     = &chip8->cpu;
    bool *keypad = chip8->keypad;
    byte *memory = chip8->memory, *display = chip8->display;

    size_t                          //
        n = (opcode & 0x000f),      //
        y = (opcode & 0x00f0) >> 4, //
        x = (opcode & 0x0f00) >> 8;

    byte kk      = (opcode & 0x00ff);
    uint16_t nnn = opcode & 0x0fff;

    chip8->should_draw = 0;

    switch ((opcode & 0xf000) >> 12) {
    case 0x0: {
        switch (n) {
        // `00E0` -> CLS
        case 0x0: {
            clear_display(chip8);
            chip8->should_draw = 1;
        } break;

        // `00EE` -> RET
        case 0xe: {
            cpu->pc = cpu->stack[--cpu->sp];
            return;
        } break;

        default: exit(2);
        }
    } break;

    // `1nnn` -> JP addr
    //    - set program counter to `nnn`.
    case 0x1: {
        cpu->pc = nnn;
        return;
    } break;

    // `2nnn` -> CALL addr
    //    - current program counter goes on top the stack.
    //    - increment stack pointer.
    //    - set program counter to `nnn`.
    case 0x2: {
        cpu->stack[cpu->sp] = cpu->pc + 2;
        cpu->sp++;
        cpu->pc = nnn;
        return;
    } break;

    // `3xkk` -> SE Vx, byte
    //    - skip next instruction if `Vx == kk`.
    case 0x3: {
        if (cpu->V[x] == kk)
            step(&chip8->cpu);
    } break;

    // `4xkk` -> SNE Vx, byte
    //    - skip next instruction if `Vx != kk`.
    case 0x4: {
        if (cpu->V[x] != kk)
            step(cpu);
    } break;

    // `5xy0` -> SE Vx, Vy
    //    - skip next instruction if `Vx == Vy`.
    case 0x5: {
        if (cpu->V[x] == cpu->V[y])
            step(cpu);
    } break;

    // `6xkk` -> LD Vx, byte
    //    - put value `kk` into register `Vx`.
    case 0x6: cpu->V[x] = kk; break;

    // `7xkk` -> ADD Vx, byte
    //    - set `Vx = Vx + kk`.
    case 0x7: cpu->V[x] = cpu->V[x] + kk; break;

    case 0x8: {
        switch (n) {
        // `8xy0` -> LD Vx, Vy
        //    - set `Vx = Vy`.
        case 0x0: cpu->V[x] = cpu->V[y]; break;

        // `8xy1` -> OR Vx, Vy
        //    - set `Vx = Vx | Vy`.
        case 0x1: cpu->V[x] |= cpu->V[y]; break;

        // `8xy2` -> AND Vx, Vy
        //    - set `Vx = Vx & Vy`.
        case 0x2: cpu->V[x] &= cpu->V[y]; break;

        // `8xy3` -> XOR Vx, Vy
        //    - set `Vx = Vx ^ Vy`.
        case 0x3: cpu->V[x] ^= cpu->V[y]; break;

        // `8xy4` -> ADD Vx, Vy
        //    - calculate `Vx + Vy`
        //    - set VF to 1, if `Vx + Vy` is more than 8 bits, else 0.
        //    - store the lower 8 bits of `Vx + Vy` into `Vx`.
        case 0x4: {
            uint16_t sum = cpu->V[x] + cpu->V[y];
            cpu->V[0xf]  = sum > 0xff;
            cpu->V[x]    = sum & 0xff;
        } break;

        // `8xy5` -> SUB Vx, Vy
        //    - set `Vx = Vx - Vy`.
        //    - set VF to 1, if `Vx > Vy`, else 0.
        case 0x5: {
            cpu->V[0xf] = cpu->V[x] > cpu->V[y];
            cpu->V[x] -= cpu->V[y];
        } break;

        // `8xy6` -> SHR Vx
        //    - set `VF` to least significant bit of `Vx`
        //    - divide `Vx` by 2.
        case 0x6: {
            cpu->V[0xf] = cpu->V[x] & 1;
            cpu->V[x] >>= 1;
        } break;

        // `8xy7` -> SUBN Vx, Vy
        //    - set `Vx = Vy - Vx`.
        //    - set VF to 1, if `Vy > Vx`, else 0.
        case 0x7: {
            cpu->V[0xf] = (cpu->V[y] > cpu->V[x]);
            cpu->V[x]   = cpu->V[y] - cpu->V[x];
        } break;

        // `8xyE` -> SHL Vx, Vy
        //    - set `VF` to most significant bit of `Vx`.
        //    - multiply `Vx` by 2
        case 0xe: {
            cpu->V[0xf] = cpu->V[x] >> 7;
            cpu->V[x] <<= 1;
        } break;

        default: exit(2);
        }
    } break;

    // `9xy0` -> SNE Vx, Vy
    //    - skip next instruction if `Vx != Vy`.
    case 0x9: {
        if (cpu->V[x] != cpu->V[y])
            step(cpu);
    } break;

    // `Annn` -> LD I, addr
    //    - set `I` to `nnn`.
    case 0xa: cpu->I = nnn; break;

    // `Bnnn` -> JP V0, addr
    //    - Jump to location `nnn + V0`.
    case 0xb: {
        cpu->pc = nnn + cpu->V[0];
        return;
    } break;

    // `Cxkk` - RND Vx, byte
    //    - set `Vx` = rand AND kk.
    case 0xc: cpu->V[x] = random_byte() & kk; break;

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
        byte Vx     = cpu->V[x];
        byte Vy     = cpu->V[y];
        cpu->V[0xf] = 0;

        for (size_t row = 0; row < n; ++row) {
            byte pixel = memory[cpu->I + row];
            byte y     = (Vy + row) % HEIGHT;
            for (size_t col = 0; col < 8; ++col) {
                if (pixel & (0x80 >> col)) {
                    byte x = (Vx + col) % WIDTH;
                    if (display[y * WIDTH + x])
                        cpu->V[0xf] = 1; // collision.
                    display[y * WIDTH + x] ^= 1;
                }
            }
        }
        chip8->should_draw = 1;
    } break;

    // `Exkk`.
    case 0xe: {
        switch (kk) {
        // `Ex9E`
        // skip next instruction if key with the value of Vx is pressed.
        // Checks the keypad, and if the key corresponding to the value
        // of Vx is currently in the down position, PC is increased by 2.
        case 0x9e: {
            if (keypad[cpu->V[x]])
                step(cpu);
        } break;
        // `ExA1` - SKNP Vx
        //    - skip next instruction if key with the value of Vx is not
        //    pressed.
        case 0xa1: {
            if (!keypad[cpu->V[x]])
                step(cpu);
        } break;
        default: exit(2);
        }
    } break;

    // `Fxkk`
    case 0xf: {
        switch (kk) {
        // `Fx07` -> LD Vx, DT
        //    - set `Vx` to `DT` value.
        case 0x07: cpu->V[x] = cpu->dt; break;

        // `Fx0A` - LD Vx, K
        //    - wait for a key press, store the value of the key in `Vx`.
        case 0x0a: {
        } break;

        // `Fx15` -> LD DT, Vx
        //    - set `DT` to `Vx` value.
        case 0x15: cpu->dt = cpu->V[x]; break;

        // `Fx18` - LD ST, Vx
        //    - Set `ST` to `Vx` value.
        case 0x18: cpu->st = cpu->V[x]; break;

        // `Fx1E` - ADD I, Vx
        //    - set `I = I + Vx`.
        case 0x1e: {
            cpu->I += cpu->V[x];
            cpu->V[0xf] = cpu->I > 0x0F00;
        } break;

        // `Fx29` - LD F, Vx
        //    - set `I` to location of sprite for digit `Vx`.
        case 0x29: cpu->I = cpu->V[x] * 5; break;

        // `Fx33` - LD B, Vx
        //    - places the hundreds digit of `Vx` at `memory[I]`.
        //    - places the tens digit of `Vx` at `memory[I + 1]`.
        //    - places the digits digit of `Vx` at `memory[I + 2]`.
        case 0x33: {
            memory[cpu->I]     = cpu->V[x] / 100;
            memory[cpu->I + 1] = (cpu->V[x] % 100) / 10;
            memory[cpu->I + 2] = cpu->V[x] % 10;
        } break;

        // `Fx55` -> LD [I], Vx
        //    - store registers V0 through Vx in memory starting at location I.
        case 0x55: memcpy(memory + cpu->I, cpu->V, x + 1); break;

        // `Fx65` -> LD Vx, [I]
        //    - read registers V0 through Vx from memory starting at location I.
        case 0x65: memcpy(cpu->V, memory + cpu->I, x + 1); break;

        default: exit(2);
        }
    } break;

    default: exit(2);
    }

    step(cpu);
}
