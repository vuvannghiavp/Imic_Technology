#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "protocol_examples_common.h" // Thư viện hỗ trợ kết nối Wi-Fi nhanh trong ví dụ IDF
#include "esp_http_client.h"

static const char *TAG = "HTTP_CLIENT_EXAMPLE";

/**
 * @brief Hàm callback xử lý các sự kiện HTTP
 * Tại đây chúng ta sẽ bắt được dữ liệu phản hồi từ server và in ra log.
 */
esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGI(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGI(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            /*
             * In nội dung response body nhận được từ server ra output logs.
             * Lưu ý: Dữ liệu nhận được có thể không kết thúc bằng '\0', nên dùng printf với độ dài.
             */
            if (!esp_http_client_is_chunked_response(evt->client)) {
                printf("%.*s", evt->data_len, (char*)evt->data);
            }
            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_FINISH");
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
            break;
        case HTTP_EVENT_REDIRECT:
            ESP_LOGI(TAG, "HTTP_EVENT_REDIRECT");
            break;
    }
    return ESP_OK;
}

/**
 * @brief Task thực hiện HTTP Request
 */
static void http_rest_with_url(void)
{
    // Cấu hình HTTP Client
    // Thư viện tự động xử lý DNS khi ta cung cấp URL dạng domain name
    esp_http_client_config_t config = {
        .url = "http://httpforever.com", // URL server test (Plain HTTP)
        .event_handler = _http_event_handler,
        .method = HTTP_METHOD_GET,
        .timeout_ms = 5000,
    };
    
    esp_http_client_handle_t client = esp_http_client_init(&config);

    // Thực hiện request
    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %lld",
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client));
    } else {
        ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
    }

    // Giải phóng tài nguyên
    esp_http_client_cleanup(client);
}

void app_main(void)
{
    // 1. Khởi tạo NVS (cần thiết cho Wi-Fi)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // 2. Khởi tạo TCP/IP Stack và Event Loop
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* 3. Kết nối Wi-Fi 
     * Lưu ý: example_connect() là hàm helper của ESP-IDF giúp kết nối nhanh tới Wi-Fi 
     * được cấu hình trong `idf.py menuconfig`.
     */
    ESP_ERROR_CHECK(example_connect());
    ESP_LOGI(TAG, "Connected to AP, now performing HTTP request...");

    // 4. Chạy demo HTTP
    http_rest_with_url();
}