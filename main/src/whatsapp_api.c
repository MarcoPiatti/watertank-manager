#include "whatsapp_api.h"

#include <stdio.h>
#include <string.h>
#include "esp_http_client.h"
#include "cJSON.h"
#include "nvs.h"
#include <secrets.h>

#define IS_WPP_C

const char * const whatsapp_template_strings[] = {
    "low_water_alert",
    "hello_world",
    "hello_world",
    "hello_world"
};

whatsapp_contact_t wpp_marco = {marco_phone, "Marco"};
whatsapp_contact_t wpp_sebas = {sebas_phone, "Sebas"};

whatsapp_config_t whatsapp_config = {
    .endpoint = NULL,
    .api_key = NULL,
    .contacts = {&wpp_marco, &wpp_sebas, NULL, NULL, NULL}
};

void whatsapp_persist(whatsapp_config_t config) {
    nvs_handle_t nvs_handle;
    nvs_open("whatsapp", NVS_READWRITE, &nvs_handle);
    nvs_set_str(nvs_handle, "endpoint", config.endpoint);
    nvs_set_str(nvs_handle, "api_key", config.api_key);
    for (int i = 0; i < 5; i++) {
        if (config.contacts[i] == NULL) { break; }
        char key[10];
        sprintf(key, "contact%d", i);
        nvs_set_blob(nvs_handle, key, config.contacts[i], sizeof(whatsapp_contact_t));
    }    
    nvs_commit(nvs_handle);
    nvs_close(nvs_handle);
}

void whatsapp_config_recover() {
    nvs_handle_t nvs_handle;
    nvs_open("whatsapp", NVS_READWRITE, &nvs_handle);
    size_t required_size = 0;
    char *endpoint = NULL;
    esp_err_t err = nvs_get_str(nvs_handle, "endpoint", NULL, &required_size);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        endpoint = malloc(strlen(default_wpp_url) + 1);
        strcpy(endpoint, default_wpp_url);
    }
    else {
        endpoint = malloc(required_size);
        nvs_get_str(nvs_handle, "endpoint", endpoint, &required_size);
    }
    whatsapp_config.endpoint = endpoint;

    required_size = 0;
    char *api_key = NULL;
    err = nvs_get_str(nvs_handle, "api_key", NULL, &required_size);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        api_key = malloc(strlen(default_api_key) + 1);
        strcpy(api_key, default_api_key);
    }
    else {
        api_key = malloc(required_size);
        nvs_get_str(nvs_handle, "api_key", api_key, &required_size);
    }
    whatsapp_config.api_key = api_key;

    for(int i = 0; i < 5; i++) {
        if (whatsapp_config.contacts[i] != NULL) { continue; }
        char key[10];
        sprintf(key, "contact%d", i);
        size_t required_size = 0;
        whatsapp_contact_t *contact = NULL;
        err = nvs_get_blob(nvs_handle, key, NULL, &required_size);
        if (err == ESP_ERR_NVS_NOT_FOUND) {
            contact = NULL;
        }
        else {
            contact = malloc(required_size);
            nvs_get_blob(nvs_handle, key, contact, &required_size);
        }
        whatsapp_config.contacts[i] = contact;
    }

}

void whatsapp_notify_all(whatsapp_event_t event) {
    for(int i = 0; i < 5; i++) {
        if (whatsapp_config.contacts[i] == NULL) { break; }
        whatsapp_notify(event, whatsapp_config.contacts[i]->phone);
    }
}

void whatsapp_notify(whatsapp_event_t event, const char * const phone) {

    const char * const template = whatsapp_template_strings[event];

    // Set the HTTP request configuration
    esp_http_client_config_t config = {
        .url = whatsapp_config.endpoint,
        .method = HTTP_METHOD_POST,
        .event_handler = NULL,
        .transport_type = HTTP_TRANSPORT_OVER_SSL,
        .port = 443
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    
    // Set the headers
    esp_http_client_set_header(client, "Authorization", whatsapp_config.api_key);
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
