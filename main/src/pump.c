#define IS_PUMP_C
#include <pump.h>

pump_t pump;

pump_t pump_init(gpio_num_t pin) {
    pump_t pump;
    pump.pin = pin;
    pump.state = PUMP_OFF;
    
    gpio_config_t pump_config = {
        .pin_bit_mask = 1ULL << pump.pin,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&pump_config);
    gpio_set_level(pump.pin, 1);

    return pump;
}

void pump_set_state(pump_t *pump, pump_state_t state) {
    pump->state = state;
    gpio_set_level(pump->pin, state==PUMP_OFF ? 0 : 1);
}