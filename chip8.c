#include "chip8.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
