#include "chip8.h"
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>

#define PROGRAM_LENGTH 39

unsigned int key_mapping[16] = {GLFW_KEY_X, GLFW_KEY_1, GLFW_KEY_2, GLFW_KEY_3,
                                GLFW_KEY_Q, GLFW_KEY_W, GLFW_KEY_E, GLFW_KEY_A,
                                GLFW_KEY_S, GLFW_KEY_D, GLFW_KEY_Z, GLFW_KEY_C,
                                GLFW_KEY_4, GLFW_KEY_R, GLFW_KEY_F, GLFW_KEY_V};

chip8 *cpu;

void key_callback(GLFWwindow *window, int key, int scancode, int action,
                  int mods) {
  for (int i = 0; i < 16; i++) {
    if (key == key_mapping[i] && action == GLFW_PRESS) {
      // set key press of i, in cpu
      cpu->key[i] = 1;
      if (cpu->halted) {
        cpu->V[cpu->waiting_key_register] = i;
      }
    } else if (key == key_mapping[i] && action == GLFW_RELEASE) {
      // release key press of i, in cpu
      cpu->key[i] = 0;
      if (i == cpu->V[cpu->waiting_key_register]) {
        cpu->halted = false;
        cpu->pc += 2;
      }
    }
  }
}

int main() {
  cpu = (chip8 *)malloc(sizeof(chip8));

  if (cpu == NULL) {
    perror("ERROR ALLOCATING MEMORY FOR CPU STRUCT");
  }

  initialise_chip8(cpu);
  load_program(cpu, "./tests/7-beep.ch8");
  glfwSetKeyCallback(cpu->window, key_callback);

  for (;;) {
    if (!cpu->halted) {
      executeCpuCycle(cpu);
      if (cpu->draw_flag) {
        drawDisplay(cpu);
        glfwSwapBuffers(cpu->window);
      }
      if (cpu->exit_flag) {
        printf("Exiting emulation with exit status %i\n", cpu->exit_status);
        return 0;
      }
    }
    glfwPollEvents();

    if (cpu->delay_timer > 0) {
      cpu->delay_timer--;
    }

    if (cpu->sound_timer > 0) {
      cpu->sound_timer--;
      if (cpu->sound_timer == 0) {
        printf("beep!\n");
        printf("\a\n");
      }
    }
  }
  return 0;
}
