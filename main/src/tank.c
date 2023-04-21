#define IS_TANK_C
#include <tank.h>

tank_t tank;

tank_t tank_init(double sensor_height_cm, double diameter_cm, double water_capacity_cm, ultrasonic_sensor_t sensor) {
    tank_t tank;
    tank.sensor_height_cm = sensor_height_cm;
    tank.diameter_cm = diameter_cm;
    tank.water_capacity_cm = water_capacity_cm;
    tank.water_level_cm = 0;
    tank.threshold_min_cm = 0;
    tank.threshold_max_cm = tank.water_capacity_cm;
    tank.sensor = sensor;
    return tank;
}

tank_state_t tank_update_water_level(tank_t *tank) {
    uint32_t measurement;
    ultrasonic_measure_cm(&(tank->sensor), tank->sensor_height_cm, &measurement);
    tank->water_level_cm = tank->sensor_height_cm - (double)measurement;

    if (tank->water_level_cm < tank->threshold_min_cm) { return TANK_LOW; }
    if (tank->water_level_cm > tank->threshold_max_cm) { return TANK_FULL; }
    
    return TANK_MEDIUM;
}