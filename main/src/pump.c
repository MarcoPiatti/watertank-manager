#define IS_PUMP_C
#include <pump.h>
#include <nvs.h>

pump_t pump;

pump_t pump_init(gpio_num_t pin) {
    pump_t pump;
    pump.pin = pin;

    nvs_handle_t nvs_handle;
    nvs_open("pump", NVS_READWRITE, &nvs_handle);
    esp_err_t err = nvs_get_u8(nvs_handle, "state", (uint8_t*)&(pump.state));
    if (err == ESP_ERR_NVS_NOT_FOUND) { pump.state = PUMP_ON; }
    nvs_close(nvs_handle);
    
    gpio_config_t pump_config = {
        .pin_bit_mask = 1ULL << pump.pin,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&pump_config);
    gpio_set_level(pump.pin, pump.state==PUMP_OFF ? 0 : 1);

    return pump;
}

void pump_set_state(pump_t *pump, pump_state_t state) {
    pump->state = state;
    gpio_set_level(pump->pin, state==PUMP_OFF ? 0 : 1);

    nvs_handle_t nvs_handle;
    nvs_open("pump", NVS_READWRITE, &nvs_handle);
    nvs_set_u8(nvs_handle, "state", pump->state);
    nvs_commit(nvs_handle);
    nvs_close(nvs_handle);
}