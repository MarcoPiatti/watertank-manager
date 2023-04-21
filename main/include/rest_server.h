#ifndef REST_SERVER_H
#define REST_SERVER_H

#include "esp_http_server.h"

esp_err_t start_rest_server(const char *base_path);

#endif // !REST_SERVER_H