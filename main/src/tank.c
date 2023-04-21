#define IS_TANK_C
#include <tank.h>
#include <nvs.h>

tank_t tank;

esp_err_t tank_recover(tank_t *tank) {
    /* Recovers tank variables from nvs except for the sensor, which is stateless */
    /* Values belonging to tank that are of type double are stored independently and as blobs */
    nvs_handle_t nvs_handle;
    nvs_open("tank", NVS_READWRITE, &nvs_handle);
    size_t required_size = sizeof(double);
    esp_err_t err = nvs_get_blob(nvs_handle, "sensor_pos_cm", &(tank->sensor_pos_cm), &required_size);
    if (err != ESP_OK) { return err; }
    nvs_get_blob(nvs_handle, "liters", &(tank->liters), &required_size);
    nvs_get_blob(nvs_handle, "water_max_cm", &(tank->water_max_cm), &required_size);
    nvs_get_blob(nvs_handle, "water_level_cm", &(tank->water_level_cm), &required_size);
    nvs_get_blob(nvs_handle, "low_pct", &(tank->low_pct), &required_size);
    nvs_get_blob(nvs_handle, "full_pct", &(tank->full_pct), &required_size);
    nvs_close(nvs_handle);
    return ESP_OK;
}

void tank_persist(tank_t *tank) {
    /* Persists tank variables to nvs except for the sensor, which is stateless */
    /* Values belonging to tank that are of type double are stored independently and as blobs */
    nvs_handle_t nvs_handle;
    nvs_open("tank", NVS_READWRITE, &nvs_handle);
    nvs_set_blob(nvs_handle, "sensor_pos_cm", &(tank->sensor_pos_cm), sizeof(double));
    nvs_set_blob(nvs_handle, "liters", &(tank->liters), sizeof(double));
    nvs_set_blob(nvs_handle, "water_max_cm", &(tank->water_max_cm), sizeof(double));
    nvs_set_blob(nvs_handle, "water_level_cm", &(tank->water_level_cm), sizeof(double));
    nvs_set_blob(nvs_handle, "low_pct", &(tank->low_pct), sizeof(double));
    nvs_set_blob(nvs_handle, "full_pct", &(tank->full_pct), sizeof(double));
    nvs_commit(nvs_handle);
    nvs_close(nvs_handle);
}

tank_t tank_init(double sensor_pos_cm, double liters, double water_max_cm, ultrasonic_sensor_t sensor) {
    tank_t tank;
    tank.sensor = sensor;
    esp_err_t err = tank_recover(&tank);
    if (err != ESP_OK) {
        tank.sensor_pos_cm = sensor_pos_cm;
        tank.liters = liters;
        tank.water_max_cm = water_max_cm;
        tank.water_level_cm = 0;
        tank.low_pct = 0.10;
        tank.full_pct = 0.6;

    }
    return tank;
}

tank_state_t tank_update_water_level(tank_t *tank) {
    uint32_t measurement;
    ultrasonic_measure_cm(&(tank->sensor), tank->sensor_pos_cm, &measurement);
    tank->water_level_cm = tank->sensor_pos_cm - (double)measurement;

    if (tank->water_level_cm < tank->water_max_cm * tank->low_pct) { return TANK_LOW; }
    if (tank->water_level_cm > tank->water_max_cm * tank->full_pct) { return TANK_FULL; }
    
    return TANK_MEDIUM;
}