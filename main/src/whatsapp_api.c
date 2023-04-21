#include "whatsapp_api.h"

#include <stdio.h>
#include <string.h>
#include "esp_http_client.h"
#include "cJSON.h"
#include <secrets.h>

#define WPP_MAX_CONTACTS 5

const char * const whatsapp_template_strings[] = {
    "low_water_alert",
    "hello_world",
    "hello_world"
};

typedef struct whatsapp_contact {
    const char * const phone;
    const char * const name;
} whatsapp_contact_t;

whatsapp_contact_t wpp_marco = {marco_phone, "Marco"};

whatsapp_contact_t* whatsapp_contacts[WPP_MAX_CONTACTS] = {
    &wpp_marco,
    NULL,
    NULL,
    NULL,
    NULL
};

void whatsapp_notify_all(whatsapp_event_t event) {
    for(int i = 0; i < WPP_MAX_CONTACTS; i++) {
        if (whatsapp_contacts[i] == NULL) { break; }
        whatsapp_notify(event, whatsapp_contacts[i]->phone);
    }
}

void whatsapp_notify(whatsapp_event_t event, const char * const phone) {

    const char * const template = whatsapp_template_strings[event];

    // Set the HTTP request configuration
    esp_http_client_config_t config = {
        .url = "https://graph.facebook.com/v16.0/112002441869599/messages",
        .method = HTTP_METHOD_POST,
        .event_handler = NULL,
        .transport_type = HTTP_TRANSPORT_OVER_SSL,
        .port = 443
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    
    // Set the headers
    esp_http_client_set_header(client, "Authorization", api_key);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    
    // Set the JSON body
    cJSON *root = cJSON_CreateObject();
        cJSON_AddStringToObject(root, "messaging_product", "whatsapp");
        cJSON_AddStringToObject(root, "to", phone);
        cJSON_AddStringToObject(root, "type", "template");
        cJSON *template_obj = cJSON_CreateObject();
            cJSON_AddStringToObject(template_obj, "name", template);
            cJSON *language_obj = cJSON_CreateObject();
                cJSON_AddStringToObject(language_obj, "code", "en_US");
            cJSON_AddItemToObject(template_obj, "language", language_obj);
        cJSON_AddItemToObject(root, "template", template_obj);
    char *json_data = cJSON_Print(root);
    cJSON_Delete(root);
    
    esp_http_client_set_post_field(client, json_data, strlen(json_data));
    
    // Send the HTTP request
    esp_http_client_perform(client);
    
    // Clean up
    esp_http_client_cleanup(client);
    free(json_data);
}
