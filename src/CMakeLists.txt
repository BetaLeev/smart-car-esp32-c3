idf_component_register(
    SRCS "main.c"
         "motor_driver.c"
         "wifi/wifi_manager.c"
         "web/web_server.c"
         "ble/ble_manager.c"
    INCLUDE_DIRS "."
                 "ble"
    REQUIRES esp_http_server esp_wifi esp_netif esp_timer driver freertos nvs_flash bt
)
