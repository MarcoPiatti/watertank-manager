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

#define DECLARE_HANDLER_POST_NUM(nameArg, keyArg, typeArg, destinationArg) \
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
        destinationArg = new_value; \
        cJSON_Delete(root); \
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

DECLARE_HANDLER_GET_NUM(tank_level_get, water_level, tank.water_level_cm)
DECLARE_HANDLER_GET_NUM(tank_capacity_get, water_capacity, tank.water_capacity_cm)
DECLARE_HANDLER_GET_NUM(threshold_max_get, threshold_max, tank.threshold_max_cm)
DECLARE_HANDLER_GET_NUM(threshold_min_get, threshold_min, tank.threshold_min_cm)

DECLARE_HANDLER_POST_NUM(threshold_max_post, threshold_max, double, tank.threshold_max_cm)
DECLARE_HANDLER_POST_NUM(threshold_min_post, threshold_min, double, tank.threshold_min_cm)

esp_err_t start_rest_server(const char *base_path)
{
    REST_CHECK(base_path, "wrong base path", err);
    rest_server_context_t *rest_context = calloc(1, sizeof(rest_server_context_t));
    REST_CHECK(rest_context, "No memory for rest context", err);
    strlcpy(rest_context->base_path, base_path, sizeof(rest_context->base_path));

    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.uri_match_fn = httpd_uri_match_wildcard;

    ESP_LOGI(REST_TAG, "Starting HTTP Server");
    REST_CHECK(httpd_start(&server, &config) == ESP_OK, "Start server failed", err_start);

    httpd_uri_t common_get_uri = {
        .uri = "/*",
        .method = HTTP_GET,
        .handler = rest_common_get_handler,
        .user_ctx = rest_context
    };
    httpd_register_uri_handler(server, &common_get_uri);

    REGISTER_HANDLER(/api/tank/level, GET, tank_level_get);
    REGISTER_HANDLER(/api/tank/capacity, GET, tank_capacity_get);
    REGISTER_HANDLER(/api/tank/threshold_max, GET, threshold_max_get);
    REGISTER_HANDLER(/api/tank/threshold_min, GET, threshold_min_get);

    REGISTER_HANDLER(/api/tank/threshold_max, POST, threshold_max_post);
    REGISTER_HANDLER(/api/tank/threshold_min, POST, threshold_min_post);

    return ESP_OK;
err_start:
    free(rest_context);
err:
    return ESP_FAIL;
}
