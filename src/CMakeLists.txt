idf_component_register(
    SRCS "main.c"
         "motor_driver.c"
         "button.c"
    INCLUDE_DIRS "."
    REQUIRES esp_timer esp_driver_ledc driver freertos
)
