#ifndef TANK_H
#define TANK_H

#include <ultrasonic.h>
#include <cJSON.h>

typedef enum tank_state {
    TANK_LOW,
    TANK_MEDIUM,
    TANK_FULL
} tank_state_t;

typedef struct tank {
    double sensor_pos_cm;
    double capacity_lts;
    double capacity_cm;
    double water_level_cm;
    double low_pct;
    double full_pct;
    ultrasonic_sensor_t sensor;
} tank_t;

typedef struct water_level {
    double water_level_cm;
    double water_level_lts;
    double water_level_pct;
} water_level_t;

#ifndef IS_TANK_C
extern tank_t tank;
#endif // !IS_TANK_C

tank_t tank_init(double sensor_pos_cm, double capacity_lts, double capacity_cm, ultrasonic_sensor_t sensor);

water_level_t tank_get_water_level(tank_t *tank);

tank_state_t tank_update_water_level(tank_t *tank);

void tank_persist(tank_t *tank);

cJSON* tank_to_json(tank_t *tank);

tank_t tank_from_json(cJSON *json);

cJSON* water_level_to_json(water_level_t water_level);

#endif // !TANK_H