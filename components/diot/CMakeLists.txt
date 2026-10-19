idf_component_register(
    SRCS "diot_button.c" "diot_led.c"
    INCLUDE_DIRS "include"
    REQUIRES fsm esp_driver_gpio
)
