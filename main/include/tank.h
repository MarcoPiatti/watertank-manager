#ifndef TANK_H
#define TANK_H

#include <ultrasonic.h>

typedef enum tank_state {
    TANK_LOW,
    TANK_MEDIUM,
    TANK_FULL
} tank_state_t;

typedef struct tank {
    double sensor_height_cm;
    double diameter_cm;
    double water_capacity_cm;
    double water_level_cm;
    double threshold_min_cm;
    double threshold_max_cm;
    ultrasonic_sensor_t sensor;
} tank_t;

#ifndef IS_TANK_C
extern tank_t tank;
#endif // !IS_TANK_C

tank_t tank_init(double sensor_height_cm, double diameter_cm, double water_capacity_cm, ultrasonic_sensor_t sensor);

tank_state_t tank_update_water_level(tank_t *tank);

#endif // !TANK_H