#ifndef VXAIR_INPUT_H
#define VXAIR_INPUT_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Types of input events
 */
typedef enum {
    VXAIR_INPUT_EV_KEY,
    VXAIR_INPUT_EV_MOUSE_MOVE,
    VXAIR_INPUT_EV_MOUSE_CLICK
} vxair_input_event_type_t;

/**
 * @brief Input event structure
 */
typedef struct {
    vxair_input_event_type_t type;
    union {
        struct {
            uint32_t keycode;
            bool pressed;
        } key;
        struct {
            int32_t dx;
            int32_t dy;
        } mouse_move;
        struct {
            uint8_t button;
            bool pressed;
        } mouse_click;
    } data;
} vxair_input_event_t;

/**
 * @brief Initialize input subsystem
 */
void vxair_input_init(void);

/**
 * @brief Poll an event from the input queue
 * @param event Output event structure
 * @return true if an event was popped, false if queue is empty
 */
bool vxair_input_poll_event(vxair_input_event_t* event);

/**
 * @brief Push an event into the input queue
 * @param event Event to push
 */
void vxair_input_push_event(const vxair_input_event_t* event);

#ifdef __cplusplus
}
#endif

#endif // VXAIR_INPUT_H
