/* HTTP Restful API Server

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include "rest_server.h"

#include <string.h>
#include <fcntl.h>
#include "esp_chip_info.h"
#include "esp_random.h"
#include "esp_log.h"
#include "esp_vfs.h"
#include "cJSON.h"
#include <nvs.h>

#include "tank.h"
#include "pump.h"

#define DECLARE_HANDLER_GET_NUM(nameArg, keyArg, valueArg) \
    static esp_err_t nameArg (httpd_req_t *req) { \
        httpd_resp_set_type(req, "application/json"); \
        cJSON *root = cJSON_CreateObject(); \
        cJSON_AddNumberToObject(root, #keyArg, valueArg ); \
        const char *response = cJSON_Print(root); \
        httpd_resp_sendstr(req, response); \
        free((void *)response); \
        cJSON_Delete(root); \
        return ESP_OK; \
    }

#define DECLARE_HANDLER_POST_NUM(nameArg, keyArg, typeArg, namespaceArg, destinationArg) \
    static esp_err_t nameArg (httpd_req_t *req) { \
        int total_len = req->content_len; \
        int cur_len = 0; \
        char *buf = ((rest_server_context_t *)(req->user_ctx))->scratch; \
        int received = 0; \
        if (total_len >= SCRATCH_BUFSIZE) { \
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long"); \
            return ESP_FAIL; \
        } \
        while (cur_len < total_len) { \
            received = httpd_req_recv(req, buf + cur_len, total_len); \
            if (received <= 0) { \
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value"); \
                return ESP_FAIL; \
            } \
            cur_len += received; \
        } \
        buf[total_len] = '\0'; \
        cJSON *root = cJSON_Parse(buf); \
        typeArg new_value = cJSON_GetObjectItem(root, #keyArg ) -> value##typeArg ; \
        namespaceArg . destinationArg = new_value; \
        cJSON_Delete(root); \
        nvs_handle_t handle; \
        nvs_open(#namespaceArg, NVS_READWRITE, &handle); \
        nvs_set_blob(handle, #destinationArg, &new_value, sizeof(typeArg)); \
        nvs_commit(handle); \
        nvs_close(handle); \
        httpd_resp_sendstr(req, "Post control value successfully"); \
        return ESP_OK; \
    }

#define REGISTER_HANDLER(uriArg, methodArg, handlerArg)             \
    do                                                              \
    {                                                               \
        httpd_uri_t handlerArg##_uri = {                            \
            .uri = #uriArg ,                                        \
            .method = HTTP_##methodArg ,                            \
            .handler = handlerArg ,                                 \
            .user_ctx = rest_context                                \
        };                                                          \
        httpd_register_uri_handler(server, & handlerArg##_uri);     \
    } while (0)


static const char *REST_TAG = "esp-rest";
#define REST_CHECK(a, str, goto_tag, ...)                                              \
    do                                                                                 \
    {                                                                                  \
        if (!(a))                                                                      \
        {                                                                              \
            ESP_LOGE(REST_TAG, "%s(%d): " str, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
            goto goto_tag;                                                             \
        }                                                                              \
    } while (0)

#define FILE_PATH_MAX (ESP_VFS_PATH_MAX + 128)
#define SCRATCH_BUFSIZE (10240)

typedef struct rest_server_context {
    char base_path[ESP_VFS_PATH_MAX + 1];
    char scratch[SCRATCH_BUFSIZE];
} rest_server_context_t;

#define CHECK_FILE_EXTENSION(filename, ext) (strcasecmp(&filename[strlen(filename) - strlen(ext)], ext) == 0)

/* Set HTTP response content type according to file extension */
static esp_err_t set_content_type_from_file(httpd_req_t *req, const char *filepath)
{
    const char *type = "text/plain";
    if (CHECK_FILE_EXTENSION(filepath, ".html")) {
        type = "text/html";
    } else if (CHECK_FILE_EXTENSION(filepath, ".js")) {
        type = "application/javascript";
    } else if (CHECK_FILE_EXTENSION(filepath, ".css")) {
        type = "text/css";
    } else if (CHECK_FILE_EXTENSION(filepath, ".png")) {
        type = "image/png";
    } else if (CHECK_FILE_EXTENSION(filepath, ".ico")) {
        type = "image/x-icon";
    } else if (CHECK_FILE_EXTENSION(filepath, ".svg")) {
        type = "text/xml";
    }
    return httpd_resp_set_type(req, type);
}

/* Send HTTP response with the contents of the requested file */
static esp_err_t rest_common_get_handler(httpd_req_t *req)
{
    char filepath[FILE_PATH_MAX];
    rest_server_context_t *rest_context = (rest_server_context_t *)req->user_ctx;
    strlcpy(filepath, rest_context->base_path, sizeof(filepath));
    if (req->uri[strlen(req->uri) - 1] == '/') {
        strlcat(filepath, "/index.html", sizeof(filepath));
    } else {
        strlcat(filepath, req->uri, sizeof(filepath));
    }
    int fd = open(filepath, O_RDONLY, 0);
    if (fd == -1) {
        ESP_LOGE(REST_TAG, "Failed to open file : %s", filepath);
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to read existing file");
        return ESP_FAIL;
    }

    set_content_type_from_file(req, filepath);

    char *chunk = rest_context->scratch;
    ssize_t read_bytes;
    do {
        /* Read file in chunks into the scratch buffer */
        read_bytes = read(fd, chunk, SCRATCH_BUFSIZE);
        if (read_bytes == -1) {
            ESP_LOGE(REST_TAG, "Failed to read file : %s", filepath);
        } else if (read_bytes > 0) {
            /* Send the buffer contents as HTTP response chunk */
            if (httpd_resp_send_chunk(req, chunk, read_bytes) != ESP_OK) {
                close(fd);
                ESP_LOGE(REST_TAG, "File sending failed!");
                /* Abort sending file */
                httpd_resp_sendstr_chunk(req, NULL);
                /* Respond with 500 Internal Server Error */
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to send file");
                return ESP_FAIL;
            }
        }
    } while (read_bytes > 0);
    /* Close file after sending complete */
    close(fd);
    ESP_LOGI(REST_TAG, "File sending complete");
    /* Respond with an empty chunk to signal HTTP response completion */
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

esp_err_t tank_get_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "application/json"); 
    cJSON *root = tank_to_json(&tank); 
    const char *response = cJSON_Print(root); 
    httpd_resp_sendstr(req, response); 
    free((void *)response); 
    cJSON_Delete(root); 
    return ESP_OK; 
}

esp_err_t pump_get_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "application/json"); 
    cJSON *root = cJSON_CreateObject();
    cJSON_AddBoolToObject(root, "pump_state", pump.state);
    const char *response = cJSON_Print(root); 
    httpd_resp_sendstr(req, response); 
    free((void *)response); 
    cJSON_Delete(root); 
    return ESP_OK; 
}

esp_err_t tank_post_handler(httpd_req_t *req) {
    char buf[1024];
    int ret, remaining = req->content_len;
    while (remaining > 0) {
        /* Read the data for the request */
        if ((ret = httpd_req_recv(req, buf, remaining)) <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                /* Retry receiving if timeout occurred */
                continue;
            }
            return ESP_FAIL;
        }
        /* Data received */
        cJSON *root = cJSON_Parse(buf);
        if (root == NULL) {
            const char *error_ptr = cJSON_GetErrorPtr();
            if (error_ptr != NULL) {
                ESP_LOGE(REST_TAG, "Error before: %s", error_ptr);
            }
            return ESP_FAIL;
        }
        tank_t new = tank_from_json(root);
        tank.sensor_pos_cm = new.sensor_pos_cm;
        tank.capacity_lts = new.capacity_lts;
        tank.capacity_cm = new.capacity_cm;
        tank.water_level_cm = new.water_level_cm;
        tank.low_pct = new.low_pct;
        tank.full_pct = new.full_pct;
        tank_persist(&tank);
        cJSON_Delete(root);
        remaining -= ret;
    }
    return ESP_OK;
}

esp_err_t water_level_get_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "application/json");
    water_level_t wl = tank_get_water_level(&tank);
    cJSON *root = water_level_to_json(wl);
    const char *response = cJSON_Print(root); 
    httpd_resp_sendstr(req, response); 
    free((void *)response); 
    cJSON_Delete(root); 
    return ESP_OK; 
}

esp_err_t start_rest_server(const char *base_path)
{
    REST_CHECK(base_path, "wrong base path", err);
    rest_server_context_t *rest_context = calloc(1, sizeof(rest_server_context_t));
    REST_CHECK(rest_context, "No memory for rest context", err);
    strlcpy(rest_context->base_path, base_path, sizeof(rest_context->base_path));

    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 16;
    config.uri_match_fn = httpd_uri_match_wildcard;

    ESP_LOGI(REST_TAG, "Starting HTTP Server");
    REST_CHECK(httpd_start(&server, &config) == ESP_OK, "Start server failed", err_start);

    httpd_uri_t water_level_get_uri = {
        .uri = "/water-level",
        .method = HTTP_GET,
        .handler = water_level_get_handler,
        .user_ctx = rest_context
    };
    httpd_register_uri_handler(server, &water_level_get_uri);
    
    httpd_uri_t tank_get_uri = {
        .uri = "/tank",
        .method = HTTP_GET,
        .handler = tank_get_handler,
        .user_ctx = rest_context
    };
    httpd_register_uri_handler(server, &tank_get_uri);

    httpd_uri_t pump_get_uri = {
        .uri = "/pump",
        .method = HTTP_GET,
        .handler = pump_get_handler,
        .user_ctx = rest_context
    };
    httpd_register_uri_handler(server, &pump_get_uri);

    httpd_uri_t common_get_uri = {
        .uri = "/*",
        .method = HTTP_GET,
        .handler = rest_common_get_handler,
        .user_ctx = rest_context
    };
    httpd_register_uri_handler(server, &common_get_uri);

    return ESP_OK;
err_start:
    free(rest_context);
err:
    return ESP_FAIL;
}
