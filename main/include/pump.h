#ifndef PUMP_H
#define PUMP_H

#include <driver/gpio.h>

typedef enum pump_state {
    PUMP_OFF,
    PUMP_ON
} pump_state_t;

typedef struct pump {
    pump_state_t state;
    gpio_num_t pin;
} pump_t;

#ifndef IS_PUMP_C
extern pump_t pump;
#endif // !IS_PUMP_C

pump_t pump_init(gpio_num_t pin);

void pump_set_state(pump_t *pump, pump_state_t state);

#endif // !PUMP_H