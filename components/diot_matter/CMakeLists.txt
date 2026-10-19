idf_component_register(
    SRCS "diot_matter_devices.cpp" "diot_matter_led.c" "diot_matter.cpp" "diot_commissioning.cpp"
    INCLUDE_DIRS "include"
    PRIV_INCLUDE_DIRS "." "${ESP_MATTER_PATH}/examples/common/utils"
    REQUIRES esp_matter diot device
)

if (CONFIG_ENABLE_SET_CERT_DECLARATION_API)
    target_add_binary_data(${COMPONENT_TARGET} "certification_declaration/certification_declaration.der" BINARY)
endif()

target_compile_options(${COMPONENT_LIB} PRIVATE "-DCHIP_HAVE_CONFIG_H")
