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
    nvs_get_blob(nvs_handle, "capacity_lts", &(tank->capacity_lts), &required_size);
    nvs_get_blob(nvs_handle, "capacity_cm", &(tank->capacity_cm), &required_size);
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
    nvs_set_blob(nvs_handle, "capacity_lts", &(tank->capacity_lts), sizeof(double));
    nvs_set_blob(nvs_handle, "capacity_cm", &(tank->capacity_cm), sizeof(double));
    nvs_set_blob(nvs_handle, "low_pct", &(tank->low_pct), sizeof(double));
    nvs_set_blob(nvs_handle, "full_pct", &(tank->full_pct), sizeof(double));
    nvs_commit(nvs_handle);
    nvs_close(nvs_handle);
}

tank_t tank_init(double sensor_pos_cm, double capacity_lts, double capacity_cm, ultrasonic_sensor_t sensor) {
    tank_t tank;
    tank.sensor = sensor;
    esp_err_t err = tank_recover(&tank);
    if (err != ESP_OK) {
        tank.sensor_pos_cm = sensor_pos_cm;
        tank.capacity_lts = capacity_lts;
        tank.capacity_cm = capacity_cm;
        tank.water_level_cm = capacity_cm / 2;
        tank.low_pct = 0.10;
        tank.full_pct = 0.6;
    }
    return tank;
}

water_level_t tank_get_water_level(tank_t *tank) {
    water_level_t res = {
        .water_level_cm = tank->water_level_cm, 
        .water_level_pct = tank->water_level_cm / tank->capacity_cm,
        .water_level_lts = tank->water_level_cm * tank->capacity_lts / tank->capacity_cm,
    }; 
    return res;
}

tank_state_t tank_update_water_level(tank_t *tank) {
    uint32_t measurement;
    ultrasonic_measure_cm(&(tank->sensor), tank->sensor_pos_cm, &measurement);
    tank->water_level_cm = tank->sensor_pos_cm - (double)measurement;

    if (tank->water_level_cm < tank->capacity_cm * tank->low_pct) { return TANK_LOW; }
    if (tank->water_level_cm > tank->capacity_cm * tank->full_pct) { return TANK_FULL; }
    
    return TANK_MEDIUM;
}

cJSON* tank_to_json(tank_t *tank) {
    cJSON *json = cJSON_CreateObject();
    cJSON_AddNumberToObject(json, "sensor_pos_cm", tank->sensor_pos_cm);
    cJSON_AddNumberToObject(json, "capacity_lts", tank->capacity_lts);
    cJSON_AddNumberToObject(json, "capacity_cm", tank->capacity_cm);
    cJSON_AddNumberToObject(json, "low_pct", tank->low_pct);
    cJSON_AddNumberToObject(json, "full_pct", tank->full_pct);
    return json;
}

tank_t tank_from_json(cJSON *json) {
    tank_t tank;
    tank.sensor_pos_cm = cJSON_GetObjectItem(json, "sensor_pos_cm")->valuedouble;
    tank.capacity_lts = cJSON_GetObjectItem(json, "capacity_lts")->valuedouble;
    tank.capacity_cm = cJSON_GetObjectItem(json, "capacity_cm")->valuedouble;
    tank.low_pct = cJSON_GetObjectItem(json, "low_pct")->valuedouble;
    tank.full_pct = cJSON_GetObjectItem(json, "full_pct")->valuedouble;
    return tank;
}

cJSON* water_level_to_json(water_level_t water_level) {
    cJSON *json = cJSON_CreateObject();
    cJSON_AddNumberToObject(json, "water_level_cm", water_level.water_level_cm);
    cJSON_AddNumberToObject(json, "water_level_pct", water_level.water_level_pct);
    cJSON_AddNumberToObject(json, "water_level_lts", water_level.water_level_lts);
    return json;
}