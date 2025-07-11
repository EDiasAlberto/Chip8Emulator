#include "chip8.h"
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>

#define PROGRAM_LENGTH 39

int main() {
  chip8 *cpu = malloc(sizeof(chip8));

  if (cpu == NULL) {
    perror("ERROR ALLOCATING MEMORY FOR CPU STRUCT");
  }

  initialise_chip8(cpu);
  load_program(cpu, "./tests/ibm-logo.ch8");

  for (;;) {
    executeCpuCycle(cpu);
    if (cpu->draw_flag) {
      drawDisplay(cpu);
      glfwSwapBuffers(cpu->window);
    }
    glfwPollEvents();
  }
  return 0;
}
