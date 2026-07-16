idf_component_register(
    SRCS "main.c"
         "motor_driver.c"
         "wifi/wifi_manager.c"
         "web/web_server.c"
    INCLUDE_DIRS "."
    REQUIRES esp_http_server esp_wifi esp_netif esp_timer driver freertos nvs_flash
)
