#include "chip8.h"
#include <GLFW/glfw3.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// TODO:
// - implement more opcodes !!

#define OVERFLOW_REG 0xF
#define OPCODE_BITMASK 0xF000
#define OPERAND_1_BITMASK 0x0F00
#define OPERAND_2_BITMASK 0x00F0
#define SUBTYPE_BITMASK 0x000F

#define PIXEL_SIZE 10
#define WINDOW_WIDTH (64 * PIXEL_SIZE)
#define WINDOW_HEIGHT (32 * PIXEL_SIZE)

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
bool doRegistersPassCondition(char num1, char num2, uint8_t comparison);
short getHexDigits(short opcode, uint8_t start_pos, uint8_t num_bits);
void pushPC(chip8 *cpu);
void popPC(chip8 *cpu);

void handleControlInstruction(chip8 *cpu);
void handleKeypressSkipInstruction(chip8 *cpu);
void handleConditionalSkipOperation(chip8 *cpu);
void handleTimerInstruction(chip8 *cpu);
void handleDrawInstruction(chip8 *cpu);

void drawDisplay(chip8 *cpu);
void clearDisplay(chip8 *cpu);

void initialise_chip8(chip8 *cpu) {
  printf("Setting program counter to 0x200 and variables to 0\n");
  cpu->pc = 0x200;
  cpu->opcode = 0;
  cpu->I = 0;
  cpu->sp = 0;
  cpu->draw_flag = false;

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

  printf("Initialising graphics\n");
  if (!glfwInit()) {
    printf(stderr, "Failed to initialise GLFW\n");
  }

  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

  GLFWwindow *window =
      glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Chip-8", NULL, NULL);

  if (!window) {
    printf(stderr, "Failed to create window\n");
    glfwTerminate();
  }

  cpu->window = window;

  glfwMakeContextCurrent(cpu->window);

  glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0, WINDOW_WIDTH, WINDOW_HEIGHT, 0, -1, 1);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glClearColor(0, 0, 0, 1);
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
  cpu->draw_flag = false;
  printf("RUNNING OPCODE: 0x%x\n", cpu->opcode);
  uint8_t target_reg;
  uint8_t source_reg;
  int imm_val;
  int rand_int;

  switch (cpu->opcode & 0xF000) {
  case 0x0000:
    handleControlInstruction(cpu);
    break;
  case 0x1000:
    cpu->pc = getHexDigits(cpu->opcode, 1, 3);
    break;
  case 0x2000:
    pushPC(cpu);
    cpu->pc = getHexDigits(cpu->opcode, 1, 3);
    break;
  case 0x3000:
    target_reg = getHexDigits(cpu->opcode, 1, 1);
    imm_val = getHexDigits(cpu->opcode, 2, 2);
    if (cpu->V[target_reg] == imm_val) {
      cpu->pc += 4;
    } else {
      cpu->pc += 2;
    }
    break;
  case 0x4000:
    target_reg = getHexDigits(cpu->opcode, 1, 1);
    imm_val = getHexDigits(cpu->opcode, 2, 2);
    if (cpu->V[target_reg] != imm_val) {
      cpu->pc += 4;
    } else {
      cpu->pc += 2;
    }
    break;
  case 0x5000:
    handleConditionalSkipOperation(cpu);
  case 0x6000:
    target_reg = getHexDigits(cpu->opcode, 1, 1);
    imm_val = cpu->opcode & 0x00FF;
    cpu->V[target_reg] = imm_val;
    cpu->pc += 2;
    break;
  case 0x7000:
    target_reg = getHexDigits(cpu->opcode, 1, 1);
    imm_val = cpu->opcode & 0x00FF;
    cpu->V[target_reg] = cpu->V[target_reg] + imm_val;
    cpu->pc += 2;
    break;
  case 0x8000:
    handleRegToRegOperation(cpu);
    cpu->pc += 2;
    break;
  case 0x9000:
    target_reg = getHexDigits(cpu->opcode, 1, 1);
    source_reg = getHexDigits(cpu->opcode, 2, 1);
    if (cpu->V[source_reg] != cpu->V[target_reg]) {
      cpu->pc += 4;
    } else {
      cpu->pc += 2;
    }
    break;
  case 0xA000:
    imm_val = cpu->opcode & 0x0FFF;
    cpu->I = imm_val;
    cpu->pc += 2;
    break;
  case 0xB000:
    imm_val = cpu->opcode & 0x0FFF;
    cpu->pc = imm_val + cpu->V[0];
    break;
  case 0xC000:
    rand_int = rand() % (255);
    imm_val = cpu->opcode & 0x00FF;
    target_reg = getHexDigits(cpu->opcode, 1, 1);
    cpu->V[target_reg] = rand_int && imm_val;
    cpu->pc += 2;
    break;
  case 0xD000:
    handleDrawInstruction(cpu);
    break;
  case 0xE000:
    handleKeypressSkipInstruction(cpu);
    break;
  case 0xF000:
    handleTimerInstruction(cpu);
    break;

  default:
    printf("Error: Unknown opcode: 0x%x\n", cpu->opcode);
  }
}

void handleControlInstruction(chip8 *cpu) {
  switch (cpu->opcode & 0x00F0) {
  case 0x0010:
    cpu->exit_flag = true;
    cpu->exit_status = getHexDigits(cpu->opcode, 3, 1);
    break;
  case 0x00E0:
    switch (cpu->opcode & 0x000F) {
    case 0x0000:
      clearDisplay(cpu);
      cpu->pc += 2;
      break;
    case 0x000E:
      popPC(cpu);
      break;
    }
    break;
  default:
    // 0x0NNN is a jump to NNN
    printf("Unknown control instruction: 0x%x\n", cpu->opcode);
  }
}

void handleKeypressSkipInstruction(chip8 *cpu) {
  switch (cpu->opcode & 0x00FF) {
  case 0x009E:
    if (cpu->key[getHexDigits(cpu->opcode, 1, 1)]) {
      cpu->pc += 4;
    } else {
      cpu->pc += 2;
    }
    break;
  case 0x00A1:

    if (cpu->key[getHexDigits(cpu->opcode, 1, 1)]) {
      cpu->pc += 2;
    } else {
      cpu->pc += 4;
    }
    break;
  }
}

void handleTimerInstruction(chip8 *cpu) {
  uint8_t target_reg = getHexDigits(cpu->opcode, 1, 1);
  switch (cpu->opcode & 0x00FF) {
  case 0x0007:
    cpu->V[target_reg] = cpu->delay_timer;
    cpu->pc += 2;
    break;
  case 0x000A:
    // Need to wait for keypress and store value of key in target_reg;
    cpu->pc += 2;
    break;
  case 0x0015:
    cpu->delay_timer = cpu->V[target_reg];
    cpu->pc += 2;
    break;
  case 0x0018:
    cpu->sound_timer = cpu->V[target_reg];
    cpu->pc += 2;
    break;
  case 0x001E:
    cpu->I = cpu->V[target_reg] + cpu->I;
    cpu->V[OVERFLOW_REG] = (cpu->I > 0x0FFF);
    cpu->pc += 2;
    break;
  default:
    printf("Not implemented opcode 0x%x\n", cpu->opcode);
  }
}

void handleRegToRegOperation(chip8 *cpu) {
  uint8_t target_reg = getHexDigits(cpu->opcode, 1, 1);
  uint8_t source_reg = getHexDigits(cpu->opcode, 2, 1);
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

void handleDrawInstruction(chip8 *cpu) {
  unsigned short x_coord = cpu->V[getHexDigits(cpu->opcode, 1, 1)];
  unsigned short y_coord = cpu->V[getHexDigits(cpu->opcode, 2, 1)];
  unsigned short row_count = getHexDigits(cpu->opcode, 3, 1);
  unsigned short pixel_value;

  cpu->V[OVERFLOW_REG] = 0;
  for (int row = 0; row < row_count; row++) {
    pixel_value = cpu->memory[cpu->I + row];
    for (int col = 0; col < 8; col++) {
      if ((pixel_value & (0x80 >> col)) != 0) {
        if (cpu->display[(x_coord + col + ((y_coord + row) * 64))] == 1) {
          cpu->V[OVERFLOW_REG] = 1;
        }
        cpu->display[x_coord + col + ((y_coord + row) * 64)] ^= 1;
      }
    }
  }

  cpu->draw_flag = true;
  cpu->pc += 2;
}

void handleConditionalSkipOperation(chip8 *cpu) {
  uint8_t target_reg = getHexDigits(cpu->opcode, 1, 1);
  uint8_t source_reg = getHexDigits(cpu->opcode, 2, 1);
  uint8_t comparisonIdentifier = getHexDigits(cpu->opcode, 3, 1);
  if (doRegistersPassCondition(cpu->V[source_reg], cpu->V[target_reg],
                               comparisonIdentifier)) {
    cpu->pc += 4;
  } else {
    cpu->pc += 2;
  }
}

bool checkOverflow(char a, char b) {
  return ((unsigned int)a + (unsigned int)b > UCHAR_MAX);
}

void drawDisplay(chip8 *cpu) {
  glClear(GL_COLOR_BUFFER_BIT);
  glColor3f(1.0f, 1.0f, 1.0f);

  glBegin(GL_QUADS);
  for (int row = 0; row < 32; ++row) {
    for (int col = 0; col < 64; ++col) {
      if (cpu->display[row * 64 + col]) {
        float x = col * PIXEL_SIZE;
        float y = row * PIXEL_SIZE;

        glVertex2f(x, y);
        glVertex2f(x + PIXEL_SIZE, y);
        glVertex2f(x + PIXEL_SIZE, y + PIXEL_SIZE);
        glVertex2f(x, y + PIXEL_SIZE);
      }
    }
  }
  glEnd();
}

void clearDisplay(chip8 *cpu) { memset(cpu->display, 0, sizeof(cpu->display)); }

short getHexDigits(short opcode, uint8_t start_pos, uint8_t num_bits) {
  uint8_t shift = 4 * (4 - (start_pos + num_bits));
  int bitmask = (1 << (4 * num_bits)) - 1;
  return (opcode >> shift) & bitmask;
}

bool doRegistersPassCondition(char num1, char num2, uint8_t comparison) {
  switch (comparison) {
  case 0x0:
    return num1 == num2;
    break;
  case 0x1:
    return num1 >= num2;
    break;
  case 0x2:
    return num1 <= num2;
    break;
  case 0x3:
    return num1 != num2;
    break;
  default:
    printf("UNEXPECTED COMPARISON VALUE 0x%x\n", comparison);
  }
}

void pushPC(chip8 *cpu) {
  cpu->sp++;
  cpu->stack[cpu->sp] = cpu->pc;
}

void popPC(chip8 *cpu) {
  cpu->pc = cpu->stack[cpu->sp];
  cpu->sp--;
  if (cpu->sp < 0)
    cpu->sp = 0;
}
