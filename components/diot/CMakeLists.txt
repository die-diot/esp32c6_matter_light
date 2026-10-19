idf_component_register(
    SRCS "diot_button.c" "diot_led.c"
    INCLUDE_DIRS "include"
    REQUIRES led_strip fsm esp_driver_gpio espressif__button
)
