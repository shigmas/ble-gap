#include "freertos/FreeRTOS.h"
#include "esp_system.h"
#include "nvs_flash.h"

/// 1. Add Headers
// Bluetooth includes
#include "esp_bt.h"

// Logging
#include "esp_log.h"

// Bluedroid headers
#include "esp_bt_main.h"

// GATTS api
#include "esp_gatt_defs.h"
#include "esp_gatts_api.h"

// For esp_ble_gatt_set_local_mtu
#include "esp_gatt_common_api.h"

// Lower level stuff, like getting the MAC address
#include "esp_bt_device.h"

// For the GAP and GATT event handlers
#include "esp_gap_ble_api.h"
/// Add Headers

// 2 Logging tag
#define BLE_APP_TAG                "BLE_START"
// Logging tag


// 5. Init the GAP advertising

static uint8_t service_uuid[32] = {
    /* LSB <--------------------------------------------------------------------------------> MSB */
    //first uuid, 16bit, [12],[13] is the value
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xEE, 0x00, 0x00, 0x00,
    //second uuid, 32bit, [12], [13], [14], [15] is the value
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00,
};

esp_ble_adv_data_t adv_data = {
    .set_scan_rsp = false,
    .include_name = true,
    .include_txpower = true,
    .min_interval = 0x20,
    .max_interval = 0x40,
    .appearance = 0x00,
    .manufacturer_len = 0,
    .p_manufacturer_data =  NULL,
    .service_data_len = 0,
    .p_service_data = NULL,
    .service_uuid_len = sizeof(service_uuid),
    .p_service_uuid = service_uuid,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};

void init_ble_advertising()
{
    esp_err_t ret = esp_ble_gap_config_adv_data(&adv_data);
    if (ret){
        ESP_LOGE(BLE_APP_TAG, "config adv data failed, error code = %x",
                 ret);  
    }
}
// 5. Init the GAP advertising

// 4. GAP handler
static uint8_t adv_config_done = 0;
#define adv_config_flag      (1 << 0)
#define scan_rsp_config_flag (1 << 1)

static esp_ble_adv_params_t my_adv_params = {
    .adv_int_min        = 0x20,
    .adv_int_max        = 0x40,
    .adv_type           = ADV_TYPE_IND,
    .own_addr_type      = BLE_ADDR_TYPE_PUBLIC,
    .channel_map        = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

static void gap_event_handler(esp_gap_ble_cb_event_t event,
                              esp_ble_gap_cb_param_t *param)
{
    switch(event) {
    case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
        ESP_LOGI(BLE_APP_TAG, "ADV DATA Set");
        adv_config_done &= (~adv_config_flag);
        if (adv_config_done == 0){
            esp_bt_dev_set_device_name(BLE_APP_TAG);
            ESP_LOGI(BLE_APP_TAG, "starting advertising");
            esp_ble_gap_start_advertising(&my_adv_params);
        }
        break;
    case ESP_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT:
        ESP_LOGI(BLE_APP_TAG, "ADV RSP DATA Set");
        adv_config_done &= (~scan_rsp_config_flag);
        if (adv_config_done == 0){
            ESP_LOGI(BLE_APP_TAG, "starting advertising");
            esp_ble_gap_start_advertising(&my_adv_params);
        }
        break;
    case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
        //advertising start complete event to indicate advertising start successfully or failed
        if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
            ESP_LOGI(BLE_APP_TAG, "Advertising start failed");
        }
        break;
    case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
        if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS) {
            ESP_LOGI(BLE_APP_TAG, "Advertising stop failed");
        }
        else {
            ESP_LOGI(BLE_APP_TAG, "Stop adv successfully");
        }
        break;
    case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
        ESP_LOGI(BLE_APP_TAG, "update connection params status = %d, min_int = %d, max_int = %d,conn_int = %d,latency = %d, timeout = %d",
                param->update_conn_params.status,
                param->update_conn_params.min_int,
                param->update_conn_params.max_int,
                param->update_conn_params.conn_int,
                param->update_conn_params.latency,
                param->update_conn_params.timeout);
        break;
    default:
        ESP_LOGI(BLE_APP_TAG,"Unhandled GAP event: %d", event);
        break;
    }
}
// 4 GAP handler


void app_main(void)
{
    // 2. Add capturing the return value
    esp_err_t ret = nvs_flash_init();

    // 3. Initialize bluetooth stack
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
	ret = esp_bt_controller_init (& bt_cfg);
	if (ret != ESP_OK) {
		ESP_LOGI(BLE_APP_TAG,"initialize controller failed");
	}

	ret = esp_bt_controller_enable (ESP_BT_MODE_BLE);
	if (ret != ESP_OK) {
		ESP_LOGI(BLE_APP_TAG,"enable controller failed");
	}

    // Initialize bluedroid
    ESP_LOGI(BLE_APP_TAG,"enable bluedroid stack");
	ret = esp_bluedroid_init ();
	if (ret != ESP_OK) {
		ESP_LOGI(BLE_APP_TAG,"init bluetooth failed");
	}
	ret = esp_bluedroid_enable ();
	if (ret != ESP_OK) {
		ESP_LOGI(BLE_APP_TAG,"enable bluetooth failed");
	}
    // Initialize bluetooth stack

    // 5. Init the GAP advertising
	ret = esp_ble_gap_register_callback (gap_event_handler);

    init_ble_advertising();
    // 5. Init the GAP advertising

}

