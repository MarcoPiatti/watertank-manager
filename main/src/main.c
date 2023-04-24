/* HTTP Restful API Server Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include "sdkconfig.h"
#include "driver/gpio.h"
#include "esp_vfs_semihost.h"
#include "esp_vfs_fat.h"
#include "esp_spiffs.h"
#include "sdmmc_cmd.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_log.h"
#include "mdns.h"
#include "lwip/apps/netbiosns.h"
#include "protocol_examples_common.h"

#include "ultrasonic.h"
#include "tank.h"
#include "pump.h"
#include "rest_server.h"
#include "whatsapp_api.h"

ESP_EVENT_DECLARE_BASE(WHATSAPP_EVENT);
ESP_EVENT_DEFINE_BASE(WHATSAPP_EVENT);

#define MDNS_INSTANCE "watertank web server"

static const char *TAG = "example";

static void initialise_mdns(void)
{
    mdns_init();
    mdns_hostname_set(CONFIG_EXAMPLE_MDNS_HOST_NAME);
    mdns_instance_name_set(MDNS_INSTANCE);

    mdns_txt_item_t serviceTxtData[] = {
        {"board", "esp32"},
        {"path", "/"}
    };

    ESP_ERROR_CHECK(mdns_service_add("ESP32-WebServer", "_http", "_tcp", 80, serviceTxtData,
                                     sizeof(serviceTxtData) / sizeof(serviceTxtData[0])));
}

#if CONFIG_EXAMPLE_WEB_DEPLOY_SF
esp_err_t init_fs(void)
{
    esp_vfs_spiffs_conf_t conf = {
        .base_path = CONFIG_EXAMPLE_WEB_MOUNT_POINT,
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = false
    };
    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return ESP_FAIL;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }
    return ESP_OK;
}
#endif

uint32_t down_seconds = 0;
double water_level_cm_old = 0;
void vTask_tank_control( void * pvParameters )
{
    const TickType_t xDelay = 1000/*ms*/ / portTICK_PERIOD_MS;
    for( ;; )
    {
        water_level_cm_old = tank.water_level_cm;
        tank_state_t tank_state = tank_update_water_level(&tank);
        if (tank_state == TANK_LOW && pump.state == PUMP_ON) { 
            pump_set_state(&pump, PUMP_OFF); 
            esp_event_post(WHATSAPP_EVENT, WPP_PUMP_IS_OFF, NULL, 0, portMAX_DELAY);
        }
        else if (tank_state == TANK_FULL && pump.state == PUMP_OFF) { 
            pump_set_state(&pump, PUMP_ON); 
            esp_event_post(WHATSAPP_EVENT, WPP_PUMP_IS_BACK_ON, NULL, 0, portMAX_DELAY);
        }

        if (tank.water_level_cm <= water_level_cm_old && tank.water_level_cm < tank.capacity_cm * 0.85) {
            down_seconds++;
        } else {
            down_seconds = 0;
        }
        if (down_seconds > 3600) {
            esp_event_post(WHATSAPP_EVENT, WPP_TANK_NOT_REFILLING, NULL, 0, portMAX_DELAY);
        }

        vTaskDelay(xDelay);
    }
}

void vTask_whatsapp_notifyall( void * pvParameters )
{
    for( ;;){
        whatsapp_notify_all((whatsapp_event_t)pvParameters);
        vTaskDelete(NULL);
    }
}

void whatsapp_event_handler(void *handler_args, esp_event_base_t base, int32_t id, void *event_data) {
    if (base != WHATSAPP_EVENT) { return; }
    ESP_LOGI(TAG, "whatsapp_event_handler: %ld", id);
    whatsapp_event_t event = (whatsapp_event_t)id;
    xTaskCreate(vTask_whatsapp_notifyall, "whatsapp_notifyall", 4096, (void*)event, 5, NULL);
}

void app_main(void) {
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_event_handler_register(WHATSAPP_EVENT, WPP_PUMP_IS_OFF, whatsapp_event_handler, NULL);
    esp_event_handler_register(WHATSAPP_EVENT, WPP_PUMP_IS_BACK_ON, whatsapp_event_handler, NULL);

    initialise_mdns();
    netbiosns_init();
    netbiosns_set_name(CONFIG_EXAMPLE_MDNS_HOST_NAME);

    ESP_ERROR_CHECK(example_connect());
    ESP_ERROR_CHECK(init_fs());

    ESP_ERROR_CHECK(start_rest_server(CONFIG_EXAMPLE_WEB_MOUNT_POINT));
    
    ultrasonic_sensor_t sensor = { .trigger_pin = GPIO_NUM_33, .echo_pin = GPIO_NUM_25};
    ultrasonic_init(&sensor);
    tank = tank_init(125, 1100, 118, sensor);
    pump = pump_init(GPIO_NUM_23);
    whatsapp_config_recover();

    xTaskCreatePinnedToCore(vTask_tank_control, "tank_control_task", 2048, NULL, 5, NULL, 1);
}
