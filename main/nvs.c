#include <stdio.h>
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"

static const char *NVS_TAG = "esp-nvs";
static const char *STORAGE_NAME = "storage";
static const char *LED_NAME = "led";
static const char *VISOR_NAME = "visor";

// Low level read API
static esp_err_t nvs_read(const char *name, uint8_t *val, esp_err_t *read_err) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open(STORAGE_NAME, NVS_READONLY, &handle);

    ESP_LOGD(NVS_TAG, "Opening Non-Volatile Storage, read-only..");
    if(err != ESP_OK) {
        ESP_LOGE(NVS_TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
        return err;
    } else {
        ESP_LOGD(NVS_TAG, "Opened! Reading value..");

        err = nvs_get_u8(handle, name, val);   
        switch (err) {
            case ESP_OK:
                ESP_LOGI(NVS_TAG, "Read value %s successfully!", name);
                break;
            case ESP_ERR_NVS_NOT_FOUND:
                *read_err = err;
                ESP_LOGI(NVS_TAG, "The value is not initialized yet!");
                break;
            default :
                *read_err = err;
                ESP_LOGE(NVS_TAG, "Error (%s) reading!", esp_err_to_name(err));
        }
    }

    ESP_LOGD(NVS_TAG, "Committing NVS updates...");
    err = nvs_commit(handle);
    if (err != ESP_OK) {
        ESP_LOGE(NVS_TAG, "Read commit failed!");
    } else {
        ESP_LOGD(NVS_TAG, "Read commit successfully!");
    }

    nvs_close(handle);

    return err;
}

// Low level write API
static esp_err_t nvs_write(const char *name, uint8_t val) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open(STORAGE_NAME, NVS_READWRITE, &handle);

    ESP_LOGD(NVS_TAG, "Opening Non-Volatile Storage, read-write..");
    if(err != ESP_OK) {
        ESP_LOGE(NVS_TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
        return err;
    } else {
        ESP_LOGD(NVS_TAG, "Opened! Reading value..");

        err = nvs_set_u8(handle, name, val);   
        if (err != ESP_OK) {
            ESP_LOGE(NVS_TAG, "Write failed!");
        } else {
            ESP_LOGI(NVS_TAG, "Write value %s successfully!", name);
        }
    }

    ESP_LOGD(NVS_TAG, "Committing NVS updates...");
    err = nvs_commit(handle);
    if (err != ESP_OK) {
        ESP_LOGE(NVS_TAG, "Write commit failed!");
    } else {
        ESP_LOGD(NVS_TAG, "Write commit successfully!");
    }

    nvs_close(handle);

    return err;
}

esp_err_t init_nvs() {
    // Initialize NVS
    ESP_LOGI(NVS_TAG, "Initializing NVS..");
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }

    // Check if any values are initialized, if not then write them:
    uint8_t led, visor;
    uint8_t defaultLed = 0;
    uint8_t defaultVisor = 0;
    esp_err_t read_err;
    err = nvs_read(LED_NAME, &led, &read_err);
    if (read_err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGI(NVS_TAG, "LED value not initalized, initializing..");
        nvs_write(LED_NAME, defaultLed);
    }
    err = nvs_read(VISOR_NAME, &visor, &read_err);
    if(read_err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGI(NVS_TAG, "Visor value not initialized, initializing..");
        nvs_write(VISOR_NAME, defaultVisor);
    }

    return err;
}

uint8_t read_led() {
    uint8_t val = 0; // default to 0
    esp_err_t read_err;
    nvs_read(LED_NAME, &val, &read_err);

    return val;
}

uint8_t read_visor() {
    uint8_t val = 0;
    esp_err_t read_err;
    nvs_read(VISOR_NAME, &val, &read_err);

    return val;
}

void write_led(uint8_t val) {
    nvs_write(LED_NAME, val);
}

void write_visor(uint8_t val) {
    nvs_write(VISOR_NAME, val);
}