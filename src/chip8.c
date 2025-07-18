#include "chip8.h"
#include <GLFW/glfw3.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// TODO:
// - implement sound timer decrement
// - implement delay timer decrement
// - implement key controls

#define OVERFLOW_REG 0xF
#define FONT_WIDTH 5

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
bool doRegistersPassCondition(int num1, int num2, enum Comparison comparison);
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
  cpu->halted = false;
  cpu->waiting_key_register = 0;

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
  case CONTROL:
    handleControlInstruction(cpu);
    break;
  case UNCOND_JUMP:
    cpu->pc = getHexDigits(cpu->opcode, 1, 3);
    break;
  case ENTER_SUBROUTINE:
    pushPC(cpu);
    cpu->pc = getHexDigits(cpu->opcode, 1, 3);
    break;
  case REG_IMM_COMP_EQ:
    target_reg = getHexDigits(cpu->opcode, 1, 1);
    imm_val = getHexDigits(cpu->opcode, 2, 2);
    if (doRegistersPassCondition(cpu->V[target_reg], imm_val, EQUAL)) {
      cpu->pc += 4;
    } else {
      cpu->pc += 2;
    }
    break;
  case REG_IMM_COMP_NE:
    target_reg = getHexDigits(cpu->opcode, 1, 1);
    imm_val = getHexDigits(cpu->opcode, 2, 2);
    if (doRegistersPassCondition(cpu->V[target_reg], imm_val, NOTEQUAL)) {
      cpu->pc += 4;
    } else {
      cpu->pc += 2;
    }
    break;
  case COND_JUMP:
    handleConditionalSkipOperation(cpu);
    break;
  case LOAD_IMM:
    target_reg = getHexDigits(cpu->opcode, 1, 1);
    imm_val = getHexDigits(cpu->opcode, 2, 2);
    cpu->V[target_reg] = imm_val;
    cpu->pc += 2;
    break;
  case ADD_IMM:
    target_reg = getHexDigits(cpu->opcode, 1, 1);
    imm_val = getHexDigits(cpu->opcode, 2, 2);
    cpu->V[target_reg] = cpu->V[target_reg] + imm_val;
    cpu->pc += 2;
    break;
  case REG_REG_OP:
    handleRegToRegOperation(cpu);
    cpu->pc += 2;
    break;
  case REG_REG_NE:
    target_reg = getHexDigits(cpu->opcode, 1, 1);
    source_reg = getHexDigits(cpu->opcode, 2, 1);
    if (doRegistersPassCondition(cpu->V[source_reg], cpu->V[target_reg],
                                 NOTEQUAL)) {
      cpu->pc += 4;
    } else {
      cpu->pc += 2;
    }
    break;
  case LOAD_IMM_I:
    imm_val = getHexDigits(cpu->opcode, 1, 3);
    cpu->I = imm_val;
    cpu->pc += 2;
    break;
  case LOAD_IMM_PC:
    imm_val = getHexDigits(cpu->opcode, 1, 3);
    cpu->pc = imm_val + cpu->V[0];
    break;
  case LOAD_RAND:
    rand_int = rand() % (255);
    imm_val = getHexDigits(cpu->opcode, 2, 2);
    target_reg = getHexDigits(cpu->opcode, 1, 1);
    cpu->V[target_reg] = rand_int && imm_val;
    cpu->pc += 2;
    break;
  case DRAW_INSTRUCTION:
    handleDrawInstruction(cpu);
    break;
  case KEYP_JUMP:
    handleKeypressSkipInstruction(cpu);
    break;
  case TIMER_INSTRUCTION:
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
      cpu->pc += 2;
      break;
    }
    break;
  default:
    // 0x0NNN is a jump to NNN
    printf("Unknown control instruction: 0x%x\n", cpu->opcode);
  }
}

void handleKeypressSkipInstruction(chip8 *cpu) {
  uint8_t target_reg = getHexDigits(cpu->opcode, 1, 1);
  switch (cpu->opcode & 0x00FF) {
  case 0x009E:
    if (cpu->key[cpu->V[target_reg]] != 0) {
      cpu->pc += 4;
    } else {
      cpu->pc += 2;
    }
    break;
  case 0x00A1:
    if (cpu->key[cpu->V[target_reg]] != 0) {
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
    cpu->halted = true;
    cpu->waiting_key_register = target_reg;
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
  case 0x0029:
    uint8_t req_char = cpu->V[target_reg];
    cpu->I = FONT_WIDTH * req_char;
    cpu->pc += 2;
    break;
  case 0x0033:
    cpu->memory[cpu->I] = cpu->V[target_reg] / 100;
    cpu->memory[cpu->I + 1] = (cpu->V[target_reg] / 10) % 10;
    cpu->memory[cpu->I + 2] = (cpu->V[target_reg] % 100) % 10;
    cpu->pc += 2;
    break;
  case 0x0055:
    for (int i = 0; i <= target_reg; i++) {
      cpu->memory[(cpu->I) + i] = cpu->V[i];
    }
    cpu->pc += 2;
    break;
  case 0x0065:
    for (int i = 0; i <= target_reg; i++) {
      cpu->V[i] = cpu->memory[(cpu->I) + i];
    }
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
  case ASSIGNMENT:
    cpu->V[target_reg] = cpu->V[source_reg];
    break;
  case BITWISE_OR:
    cpu->V[target_reg] = cpu->V[target_reg] | cpu->V[source_reg];
    break;
  case BITWISE_AND:
    cpu->V[target_reg] = cpu->V[target_reg] & cpu->V[source_reg];
    break;
  case BITWISE_XOR:
    cpu->V[target_reg] = cpu->V[target_reg] ^ cpu->V[source_reg];
    break;
  case ADD:
    overflow_flag = checkOverflow(cpu->V[target_reg], cpu->V[source_reg]);
    cpu->V[OVERFLOW_REG] = overflow_flag;
    cpu->V[target_reg] = cpu->V[target_reg] + cpu->V[source_reg];
    break;
  case SUBTRACT:
    cpu->V[OVERFLOW_REG] = cpu->V[target_reg] > cpu->V[source_reg];
    cpu->V[target_reg] = cpu->V[target_reg] - cpu->V[source_reg];
    break;
  case BITSHIFT_R:
    cpu->V[OVERFLOW_REG] = cpu->V[target_reg] & 0x1;
    cpu->V[target_reg] = cpu->V[target_reg] >> 1;
    break;
  case REVERSE_SUB:
    cpu->V[OVERFLOW_REG] = cpu->V[source_reg] > cpu->V[target_reg];
    cpu->V[target_reg] = cpu->V[source_reg] - cpu->V[target_reg];
    break;
  case BITSHIFT_L:
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

bool doRegistersPassCondition(int num1, int num2, enum Comparison comparison) {
  switch (comparison) {
  case EQUAL:
    return num1 == num2;
    break;
  case GREATERTHAN:
    return num1 >= num2;
    break;
  case LESSTHAN:
    return num1 <= num2;
    break;
  case NOTEQUAL:
    return num1 != num2;
    break;
  default:
    printf("UNEXPECTED COMPARISON VALUE 0x%x\n", comparison);
  }
}

void pushPC(chip8 *cpu) {
  cpu->stack[cpu->sp] = cpu->pc;
  cpu->sp++;
}

void popPC(chip8 *cpu) {
  cpu->sp--;
  if (cpu->sp < 0)
    cpu->sp = 0;
  cpu->pc = cpu->stack[cpu->sp];
}
