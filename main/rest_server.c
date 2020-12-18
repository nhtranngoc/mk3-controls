/* HTTP Restful API Server

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include "esp_netif.h"
#include "esp_eth.h"
#include "protocol_examples_common.h"

#include <esp_https_server.h>
#include "esp_vfs.h"
#include "keep_alive.h"
#include "cJSON.h"

void visor_set_state(int state);
void led_set_duty(uint8_t duty, int time);
uint8_t read_led();
uint8_t read_visor();
void write_led(uint8_t val);
void write_visor(uint8_t val);

#if !CONFIG_HTTPD_WS_SUPPORT
#error This firmware cannot be used unless HTTPD_WS_SUPPORT is enabled in esp-http-server component configuration
#endif

#define TEXT_GET_STATE      'g' 
#define TEXT_SET_STATE      's'
#define TEXT_TYPE_LED       'l'
#define TEXT_TYPE_VISOR     'v'

httpd_handle_t server = NULL;

struct async_resp_arg {
    httpd_handle_t hd;
    int fd;
    char msg[5]; // constant msg size
};
static const size_t max_clients = 4;

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

static void send_text(httpd_req_t *req, char *text) {
    httpd_ws_frame_t ws_pkt;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.payload = (uint8_t*)text;
    ws_pkt.len = strlen(text);
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;

    httpd_ws_send_frame(req, &ws_pkt);
}

static void send_ping(void *arg)
{
    struct async_resp_arg *resp_arg = arg;
    httpd_handle_t hd = resp_arg->hd;
    int fd = resp_arg->fd;
    httpd_ws_frame_t ws_pkt;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.payload = NULL;
    ws_pkt.len = 0;
    ws_pkt.type = HTTPD_WS_TYPE_PING;

    httpd_ws_send_frame_async(hd, fd, &ws_pkt);
    free(resp_arg);
}

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

esp_err_t wss_open_fd(httpd_handle_t hd, int sockfd)
{
    ESP_LOGI(REST_TAG, "New client connected %d", sockfd);
    wss_keep_alive_t h = httpd_get_global_user_ctx(hd);
    return wss_keep_alive_add_client(h, sockfd);
}

void wss_close_fd(httpd_handle_t hd, int sockfd)
{
    ESP_LOGI(REST_TAG, "Client disconnected %d", sockfd);
    wss_keep_alive_t h = httpd_get_global_user_ctx(hd);
    wss_keep_alive_remove_client(h, sockfd);
}

static void send_text_with_custom_arg(void *arg) {
    struct async_resp_arg *resp_arg = (struct async_resp_arg *) arg;
    httpd_handle_t hd = resp_arg->hd;
    int fd = resp_arg->fd;
    httpd_ws_frame_t ws_pkt;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    char payload[5];
    memcpy(payload, resp_arg->msg, sizeof(payload));
    payload[4] = '\0';
    ws_pkt.payload = (uint8_t *)payload;
    ws_pkt.len = strlen(payload);
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;

    httpd_ws_send_frame_async(hd, fd, &ws_pkt);
    free(resp_arg);
}

// Get all clients and send async message
static void wss_broadcast(char *msg) {
    if(!server) {
        return;
    }

    ESP_LOGD(REST_TAG, "Broadcasting string: %s", msg);

    size_t clients = max_clients;
    int client_fds[max_clients];
    if (httpd_get_client_list(server, &clients, client_fds) == ESP_OK) {
        for (size_t i=0; i < clients; ++i) {
            int sock = client_fds[i];
            if (httpd_ws_get_fd_info(server, sock) == HTTPD_WS_CLIENT_WEBSOCKET) {
                ESP_LOGI(REST_TAG, "Active client (fd=%d) -> sending async message", sock);
                struct async_resp_arg *resp_arg = malloc(sizeof(struct async_resp_arg));
                memcpy(resp_arg->msg, msg, sizeof(&msg));
                resp_arg->hd = server;
                resp_arg->fd = sock;
                if (httpd_queue_work(resp_arg->hd, send_text_with_custom_arg, resp_arg) != ESP_OK) {
                    ESP_LOGE(REST_TAG, "httpd_queue_work failed!");
                    break;
                }
            }
        }
    } else {
        ESP_LOGE(REST_TAG, "httpd_get_client_list failed!");
        return;
    }
}

esp_err_t wss_handle_text_message(httpd_req_t *req, httpd_ws_frame_t *frame) {
   if (frame->len == 0 ) {
       ESP_LOGE(REST_TAG, "Invalid WebSocket message");
       // Invalid frame
       return ESP_FAIL;
   }
    switch(frame->payload[0]) {
        case TEXT_GET_STATE: {
            // If we're only getting a generic "GET" command, send everything
            uint8_t led_state = read_led();
            uint8_t visor_state = read_visor();
            char led_buffer[5], visor_buffer[5];
            // Format strings
            snprintf(led_buffer, 5, "l%hhu", led_state);
            snprintf(visor_buffer, 5, "v%hhu", visor_state);
            if (frame->len == 1) {
                ESP_LOGI(REST_TAG, "Received GET_ALL_STATE message");
                send_text(req, led_buffer);
                send_text(req, visor_buffer);
            } else {
                if (frame->payload[1] == TEXT_TYPE_LED) {
                    ESP_LOGI(REST_TAG, "Received GET_LED_STATE message");
                    send_text(req, led_buffer);
                } else if (frame->payload[1] == TEXT_TYPE_VISOR) {
                    ESP_LOGI(REST_TAG, "Received GET_VISOR_STATE message");
                    send_text(req, visor_buffer);
                }
            }
        }
        break;
        case TEXT_SET_STATE: {
            if (frame->len == 1) {
                // Invalid set command
                ESP_LOGE(REST_TAG, "Invalid SET_STATE message");
                return ESP_FAIL;
            } else {
                char set_val_str[4]; 
                char buffer[5];
                memcpy(set_val_str, frame->payload + 2, sizeof(set_val_str));
                set_val_str[3] = '\0'; // Manual null terminator just in case
                uint8_t set_val_int = atoi(set_val_str);
                if (frame->payload[1] == TEXT_TYPE_LED) {
                    ESP_LOGI(REST_TAG, "Received SET_LED message: %d", set_val_int);
                    write_led(set_val_int);
                    led_set_duty(set_val_int, 500); // fade in 500s period.
                    snprintf(buffer, 5, "l%hhu", set_val_int);
                    buffer[4] = '\0';
                    wss_broadcast(buffer);
                    send_text(req, "okl");
                } else if (frame->payload[1] == TEXT_TYPE_VISOR) {
                    ESP_LOGI(REST_TAG, "Received SET_VISOR message: %d", set_val_int);
                    write_visor(set_val_int);
                    visor_set_state(set_val_int);
                    snprintf(buffer, 5, "v%hhu", set_val_int);
                    buffer[4] = '\0';
                    wss_broadcast(buffer);
                    send_text(req, "okv");
                }
            }
        }
        break;
    }

    return ESP_OK;
}

/* ==================================================
 * ================= HANDLERS =======================
 * ================================================== 
 */

static void stop_rest_server(httpd_handle_t server)
{
    // Stop keep alive thread
    wss_keep_alive_stop(httpd_get_global_user_ctx(server));
    // Stop the httpd server
    httpd_ssl_stop(server);
}

/* Handle WS messages */
static esp_err_t ws_handler(httpd_req_t *req)
{
    uint8_t buf[128] = { 0 };
    httpd_ws_frame_t ws_pkt;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.payload = buf;

    // First receive the full ws message
    esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 128);
    if (ret != ESP_OK) {
        ESP_LOGE(REST_TAG, "httpd_ws_recv_frame failed with %d", ret);
        return ret;
    }

    // If it was a PONG, update the keep-alive
    if (ws_pkt.type == HTTPD_WS_TYPE_PONG) {
        ESP_LOGD(REST_TAG, "Received PONG message");
        return wss_keep_alive_client_is_active(httpd_get_global_user_ctx(req->handle),
                httpd_req_to_sockfd(req));

    // If it was a TEXT message, just echo it back
    } else if (ws_pkt.type == HTTPD_WS_TYPE_TEXT) {
        ESP_LOGI(REST_TAG, "Received packet with message: %s", ws_pkt.payload);
        ret = wss_handle_text_message(req, &ws_pkt);

        if (ret != ESP_OK) {
            ESP_LOGE(REST_TAG, "httpd_ws_send_frame failed with %d", ret);
        }
        ESP_LOGI(REST_TAG, "ws_handler: httpd_handle_t=%p, sockfd=%d, client_info:%d", req->handle,
                 httpd_req_to_sockfd(req), httpd_ws_get_fd_info(req->handle, httpd_req_to_sockfd(req)));
        return ret;
    }
    return ESP_OK;
}


/* Send HTTP response with the contents of the requested file */
static esp_err_t rest_common_get_handler(httpd_req_t *req)
{
    char filepath[FILE_PATH_MAX];

    rest_server_context_t *rest_context = (rest_server_context_t *)req->user_ctx;
    strlcpy(filepath, rest_context->base_path, sizeof(filepath));
    if (req->uri[strlen(req->uri) - 1] == '/') {
        strlcat(filepath, "/output.html", sizeof(filepath));
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

/* Simple handler for light brightness control */
static esp_err_t light_brightness_post_handler(httpd_req_t *req)
{
    int total_len = req->content_len;
    int cur_len = 0;
    char *buf = ((rest_server_context_t *)(req->user_ctx))->scratch;
    int received = 0;
    if (total_len >= SCRATCH_BUFSIZE) {
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long");
        return ESP_FAIL;
    }
    while (cur_len < total_len) {
        received = httpd_req_recv(req, buf + cur_len, total_len);
        if (received <= 0) {
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
            return ESP_FAIL;
        }
        cur_len += received;
    }
    buf[total_len] = '\0';

    cJSON *root = cJSON_Parse(buf);
    int led = cJSON_GetObjectItem(root, "led")->valueint;
    ESP_LOGI(REST_TAG, "Light control: %d", led);
    cJSON_Delete(root);
    httpd_resp_sendstr(req, "Post control value successfully");
    return ESP_OK;
}

/* Simple handler for visor state control */
static esp_err_t visor_state_post_handler(httpd_req_t *req)
{
    int total_len = req->content_len;
    int cur_len = 0;
    char *buf = ((rest_server_context_t *)(req->user_ctx))->scratch;
    int received = 0;
    if (total_len >= SCRATCH_BUFSIZE) {
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long");
        return ESP_FAIL;
    }
    while (cur_len < total_len) {
        received = httpd_req_recv(req, buf + cur_len, total_len);
        if (received <= 0) {
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
            return ESP_FAIL;
        }
        cur_len += received;
    }
    buf[total_len] = '\0';

    cJSON *root = cJSON_Parse(buf);
    int state = cJSON_GetObjectItem(root, "isVisorOpen")->valueint;
    ESP_LOGI(REST_TAG, "Visor state: = %d", state);
    visor_set_state(state);
    cJSON_Delete(root);
    httpd_resp_sendstr(req, "Post control value successfully");
    return ESP_OK;
}

/* Simple handler for getting system handler */
static esp_err_t system_info_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/json");
    cJSON *root = cJSON_CreateObject();
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    cJSON_AddStringToObject(root, "version", IDF_VER);
    cJSON_AddNumberToObject(root, "cores", chip_info.cores);
    const char *sys_info = cJSON_Print(root);
    httpd_resp_sendstr(req, sys_info);
    free((void *)sys_info);
    cJSON_Delete(root);
    return ESP_OK;
}
/**
 * ========================================
*/

bool client_not_alive_cb(wss_keep_alive_t h, int fd)
{
    ESP_LOGE(REST_TAG, "Client not alive, closing fd %d", fd);
    httpd_sess_trigger_close(wss_keep_alive_get_user_ctx(h), fd);
    return true;
}

bool check_client_alive_cb(wss_keep_alive_t h, int fd)
{
    ESP_LOGI(REST_TAG, "Checking if client (fd=%d) is alive", fd);
    struct async_resp_arg *resp_arg = malloc(sizeof(struct async_resp_arg));
    resp_arg->hd = wss_keep_alive_get_user_ctx(h);
    resp_arg->fd = fd;

    if (httpd_queue_work(resp_arg->hd, send_ping, resp_arg) == ESP_OK) {
        return true;
    }
    return false;
}

esp_err_t start_rest_server(const char *base_path)
{
  // Prepare keep-alive engine
    wss_keep_alive_config_t keep_alive_config = KEEP_ALIVE_CONFIG_DEFAULT();
    keep_alive_config.max_clients = max_clients;
    keep_alive_config.client_not_alive_cb = client_not_alive_cb;
    keep_alive_config.check_client_alive_cb = check_client_alive_cb;
    wss_keep_alive_t keep_alive = wss_keep_alive_start(&keep_alive_config);

    REST_CHECK(base_path, "wrong base path", err);
    rest_server_context_t *rest_context = calloc(1, sizeof(rest_server_context_t));
    REST_CHECK(rest_context, "No memory for rest context", err);
    strlcpy(rest_context->base_path, base_path, sizeof(rest_context->base_path));

    ESP_LOGI(REST_TAG, "Starting HTTP + WS Server");

    httpd_ssl_config_t conf = HTTPD_SSL_CONFIG_DEFAULT();
    conf.httpd.max_open_sockets = max_clients;
    conf.httpd.global_user_ctx = keep_alive;
    conf.httpd.open_fn = wss_open_fd;
    conf.httpd.close_fn = wss_close_fd;

    extern const unsigned char cacert_pem_start[] asm("_binary_cacert_pem_start");
    extern const unsigned char cacert_pem_end[]   asm("_binary_cacert_pem_end");
    conf.cacert_pem = cacert_pem_start;
    conf.cacert_len = cacert_pem_end - cacert_pem_start;

    extern const unsigned char prvtkey_pem_start[] asm("_binary_prvtkey_pem_start");
    extern const unsigned char prvtkey_pem_end[]   asm("_binary_prvtkey_pem_end");
    conf.prvtkey_pem = prvtkey_pem_start;
    conf.prvtkey_len = prvtkey_pem_end - prvtkey_pem_start;

    // conf.uri_match_fn = httpd_uri_match_wildcard;

    REST_CHECK(httpd_ssl_start(&server, &conf) == ESP_OK, "Start server failed", err_start);

    /* ==================================================
    * ============== URI HANDLERS ======================
    * ================================================== 
    */
    ESP_LOGI(REST_TAG, "Registering URI handlers");

    /* URI handler for fetching system info */
    httpd_uri_t system_info_get_uri = {
        .uri = "/api/v1/system/info",
        .method = HTTP_GET,
        .handler = system_info_get_handler,
        .user_ctx = rest_context
    };
    httpd_register_uri_handler(server, &system_info_get_uri);

    /* URI handler for light brightness control */
    httpd_uri_t light_brightness_post_uri = {
        .uri = "/api/v1/light/brightness",
        .method = HTTP_POST,
        .handler = light_brightness_post_handler,
        .user_ctx = rest_context
    };
    httpd_register_uri_handler(server, &light_brightness_post_uri);

    /* URI handler for visor state control */
    httpd_uri_t visor_state_post_uri = {
        .uri = "/api/v1/visor/state",
        .method = HTTP_POST,
        .handler = visor_state_post_handler,
        .user_ctx = rest_context
    };
    httpd_register_uri_handler(server, &visor_state_post_uri);


    /* URI handler for getting web server files */
    httpd_uri_t common_get_uri = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = rest_common_get_handler,
        .user_ctx = rest_context
    };
    httpd_register_uri_handler(server, &common_get_uri);

    /* URI handler for websocket */
    httpd_uri_t ws = {
        .uri        = "/ws",
        .method     = HTTP_GET,
        .handler    = ws_handler,
        .user_ctx   = NULL,
        .is_websocket = true,
        .handle_ws_control_frames = true
    };
    httpd_register_uri_handler(server, &ws);
    wss_keep_alive_set_user_ctx(keep_alive, server);


    return ESP_OK;
err_start:
    free(rest_context);
err:
    return ESP_FAIL;
}

/* Handle WSS on disconnect */
static void disconnect_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    httpd_handle_t* server = (httpd_handle_t*) arg;
    if (*server) {
        stop_rest_server(*server);
        *server = NULL;
    }
}

/* Handle WSS on connect */
static void connect_handler(void* arg, esp_event_base_t event_base,
                            int32_t event_id, void* event_data)
{
    httpd_handle_t* server = (httpd_handle_t*) arg;
    if (*server == NULL) {
        *server = (void *) start_rest_server(CONFIG_EXAMPLE_WEB_MOUNT_POINT);
    }
}