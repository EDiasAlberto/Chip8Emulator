#include "chip8.h"
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define OVERFLOW_REG 0xF

unsigned char chip8_fontset[80] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

void handleRegToRegOperation(chip8 *cpu);
bool checkOverflow(char a, char b);

void initialise_chip8(chip8 *cpu) {
  printf("Setting program counter to 0x200 and variables to 0\n");
  cpu->pc = 0x200;
  cpu->opcode = 0;
  cpu->I = 0;
  cpu->sp = 0;

  printf("Clearing arrays for display, mem, stack, regs\n");
  memset(cpu->display, 0, sizeof(cpu->display));
  memset(cpu->stack, 0, sizeof(cpu->stack));
  memset(cpu->V, 0, sizeof(cpu->V));
  memset(cpu->memory, 0, sizeof(cpu->memory));
  memset(cpu->key, 0, sizeof(cpu->key));

  printf("Loading font set\n");
  for (int i = 0; i < 80; i++) {
    cpu->memory[i] = chip8_fontset[i];
  }

  printf("Clearing timer\n");
  cpu->delay_timer = 0;
  cpu->sound_timer = 0;
}

void load_program(chip8 *cpu, char *program_name) {
  FILE *file = fopen(program_name, "rb");
  if (file == NULL) {
    perror("ERROR OPENING PROGRAM FILE");
  }

  fseek(file, 0, SEEK_END);
  long filesize = ftell(file);
  fseek(file, 0, SEEK_SET);

  char *buffer = malloc(filesize + 1);

  if (buffer == NULL) {
    perror("ERROR ALLOCATING MEMORY FOR FILE BUFFER");
  }

  fread(buffer, filesize, 1, file);
  fclose(file);
  buffer[filesize] = 0;

  for (int i = 0; i < filesize + 1; ++i) {
    cpu->memory[i + 512] = buffer[i];
  }
  free(buffer);
}

void executeCpuCycle(chip8 *cpu) {
  cpu->opcode = cpu->memory[cpu->pc] << 8 | cpu->memory[cpu->pc + 1];
  uint8_t target_reg;
  uint8_t source_reg;
  uint8_t imm_val;

  switch (cpu->opcode & 0xF000) {
  case 0x1000:
    cpu->pc = cpu->opcode & 0x0FFF;
    break;
  case 0x6000:
    target_reg = cpu->opcode & 0x0F00;
    imm_val = cpu->opcode & 0x00FF;
    cpu->V[target_reg] = imm_val;
    cpu->pc += 2;
    break;
  case 0x7000:
    target_reg = cpu->opcode & 0x0F00;
    imm_val = cpu->opcode & 0x00FF;
    cpu->V[target_reg] = cpu->V[target_reg] + imm_val;
    cpu->pc += 2;
    break;
  case 0x8000:
    handleRegToRegOperation(cpu);
    cpu->pc += 2;
    break;
  case 0x9000:
    target_reg = cpu->opcode & OPERAND_1_BITMASK;
    source_reg = cpu->opcode & OPERAND_2_BITMASK;
    if (cpu->V[source_reg] != cpu->V[target_reg]) {
      cpu->pc += 4;
    } else {
      cpu->pc += 2;
    }
    break;
  default:
    printf("Error: Unknown opcode: 0x%x\n", cpu->opcode);
  }
}

void handleRegToRegOperation(chip8 *cpu) {
  uint8_t target_reg = cpu->opcode & 0x0F00;
  uint8_t source_reg = cpu->opcode & 0x00F0;
  bool overflow_flag;

  switch (cpu->opcode & 0x000F) {
  case 0x0000:
    cpu->V[target_reg] = cpu->V[source_reg];
    break;
  case 0x0001:
    cpu->V[target_reg] = cpu->V[target_reg] | cpu->V[source_reg];
    break;
  case 0x0002:
    cpu->V[target_reg] = cpu->V[target_reg] & cpu->V[source_reg];
    break;
  case 0x0003:
    cpu->V[target_reg] = cpu->V[target_reg] ^ cpu->V[source_reg];
    break;
  case 0x0004:
    overflow_flag = checkOverflow(cpu->V[target_reg], cpu->V[source_reg]);
    cpu->V[OVERFLOW_REG] = overflow_flag;
    cpu->V[target_reg] = cpu->V[target_reg] + cpu->V[source_reg];
    break;
  case 0x0005:
    cpu->V[OVERFLOW_REG] = cpu->V[target_reg] > cpu->V[source_reg];
    cpu->V[target_reg] = cpu->V[target_reg] - cpu->V[source_reg];
    break;
  case 0x0006:
    cpu->V[OVERFLOW_REG] = cpu->V[target_reg] & 0x1;
    cpu->V[target_reg] = cpu->V[target_reg] >> 1;
    break;
  case 0x0007:
    cpu->V[OVERFLOW_REG] = cpu->V[source_reg] > cpu->V[target_reg];
    cpu->V[target_reg] = cpu->V[source_reg] - cpu->V[target_reg];
    break;
  case 0x000E:
    cpu->V[OVERFLOW_REG] = cpu->V[target_reg] & 0x80;
    cpu->V[target_reg] = cpu->V[target_reg] << 1;
    break;
  default:
    printf("Error: Unknown reg to reg operation: 0x%x\n", cpu->opcode);
  }
}

bool checkOverflow(char a, char b) {
  return ((unsigned int)a + (unsigned int)b > UCHAR_MAX);
}
