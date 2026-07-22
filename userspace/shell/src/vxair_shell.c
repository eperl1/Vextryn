#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "vxair_libc.h"
#include "vxair_shell.h"

/**
 * @file vxair_shell.c
 * @brief Minimal Shell for Vextryn Air OS
 */

#define VXAIR_MAX_COMMAND_LEN 256
#define VXAIR_MAX_ARGS 32

/**
 * @brief Prints the shell prompt.
 */
void vxair_print_prompt(void) {
    vxair_print("vxsh> ");
}

/**
 * @brief Reads a line from standard input.
 *
 * @param buffer The buffer to store the read line.
 * @param max_len The maximum number of characters to read.
 */
void vxair_read_line(char* buffer, size_t max_len) {
    size_t i = 0;
    while (i < max_len - 1) {
        int c = vxair_getchar();
        if (c < 0 || c == '\n') {
            break;
        }
        // Handle basic backspace
        if (c == '\b' || c == 127) {
            if (i > 0) {
                i--;
                vxair_print("\b \b");
            }
            continue;
        }
        buffer[i++] = (char)c;
        vxair_putchar(c); // Echo
    }
    vxair_print("\n");
    buffer[i] = '\0';
}

/**
 * @brief Parses and executes a command string.
 *
 * @param command The command string to execute.
 */
void vxair_parse_and_execute(char* command) {
    char* args[VXAIR_MAX_ARGS];
    int arg_count = 0;
    
    // Tokenize the command string into arguments
    char* token = vxair_strtok(command, " ");
    while (token != NULL && arg_count < VXAIR_MAX_ARGS - 1) {
        args[arg_count++] = token;
        token = vxair_strtok(NULL, " ");
    }
    args[arg_count] = NULL;
    
    if (arg_count == 0) {
        return;
    }
    
    // Implement built-in commands
    if (vxair_strcmp(args[0], "exit") == 0) {
        vxair_exit(0);
    } else if (vxair_strcmp(args[0], "cd") == 0) {
        vxair_puts("cd: Not implemented yet");
    } else if (vxair_strcmp(args[0], "echo") == 0) {
        for (int i = 1; i < arg_count; i++) {
            vxair_print(args[i]);
            if (i < arg_count - 1) {
                vxair_print(" ");
            }
        }
        vxair_print("\n");
    } else {
        // Spawn external programs using system calls
        int pid = vxair_spawn_process(args[0], args);
        if (pid < 0) {
            vxair_printf("vxsh: command not found: %s\n", args[0]);
        } else {
            int status = 0;
            vxair_wait(&status);
        }
    }
}

/**
 * @brief Main entry point for the shell.
 *
 * @param argc Number of arguments.
 * @param argv Array of arguments.
 * @return int Exit status.
 */
int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    
    char cmd_buffer[VXAIR_MAX_COMMAND_LEN];
    
    // Initialize shell environment
    vxair_puts("Starting vxsh (Vextryn Air Shell)...");
    
    while (true) {
        vxair_print_prompt();
        
        vxair_read_line(cmd_buffer, VXAIR_MAX_COMMAND_LEN);
        
        // Handle empty input
        if (cmd_buffer[0] == '\0') {
            continue;
        }
        
        vxair_parse_and_execute(cmd_buffer);
    }
    
    return 0;
}
