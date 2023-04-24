#ifndef WHATSAPP_API_H
#define WHATSAPP_API_H

#include "stddef.h"
#include "esp_event.h"
#include "cJSON.h"

typedef enum whatsapp_event {
    WPP_PUMP_IS_OFF,
    WPP_PUMP_IS_BACK_ON,
    WPP_TANK_NOT_REFILLING,
    WPP_HELLO_WORLD
} whatsapp_event_t;

typedef struct whatsapp_contact {
    const char * phone;
    const char * name;
} whatsapp_contact_t;

typedef struct whatsapp_config {
    char * endpoint;
    char * api_key;
    whatsapp_contact_t * contacts[5];
} whatsapp_config_t;

#ifndef IS_WPP_C
extern whatsapp_config_t whatsapp_config;
#endif // !IS_WPP_C

void whatsapp_persist(whatsapp_config_t config);

void whatsapp_config_recover();

void whatsapp_event_handler(void *handler_args, esp_event_base_t base, int32_t id, void *event_data);

void whatsapp_notify(whatsapp_event_t event, const char * const phone);

void whatsapp_notify_all(whatsapp_event_t event);


#endif // !WHATSAPP_API_H