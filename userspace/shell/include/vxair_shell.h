#ifndef VXAIR_SHELL_H
#define VXAIR_SHELL_H

#ifdef __cplusplus
extern "C" {
#endif

// Shell definitions
void vxair_print_prompt(void);
void vxair_parse_and_execute(char* command);

#ifdef __cplusplus
}
#endif

#endif // VXAIR_SHELL_H
