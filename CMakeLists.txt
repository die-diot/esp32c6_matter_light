cmake_minimum_required(VERSION 3.22)

foreach(_MATTER_ENV_VAR ESP_MATTER_PATH ESP_MATTER_DEVICE_PATH _PW_ACTUAL_ENVIRONMENT_ROOT ZAP_INSTALL_PATH)
    if(DEFINED ENV{${_MATTER_ENV_VAR}})
        string(REPLACE "\${workspaceFolder}" "${CMAKE_CURRENT_LIST_DIR}" _MATTER_ENV_VALUE "$ENV{${_MATTER_ENV_VAR}}")
        set(ENV{${_MATTER_ENV_VAR}} "${_MATTER_ENV_VALUE}")
    endif()
endforeach()

set(_DEFAULT_ESP_MATTER_PATH "${CMAKE_CURRENT_LIST_DIR}/../esp-matter")

if(NOT DEFINED ENV{ESP_MATTER_PATH} OR "$ENV{ESP_MATTER_PATH}" STREQUAL "" OR "$ENV{ESP_MATTER_PATH}" MATCHES "^\\$\\{env:")
    if(EXISTS "${_DEFAULT_ESP_MATTER_PATH}/CMakeLists.txt")
        set(ENV{ESP_MATTER_PATH} "${_DEFAULT_ESP_MATTER_PATH}")
    else()
        message(FATAL_ERROR "Please set ESP_MATTER_PATH to the path of esp-matter repo")
    endif()
endif()

if(NOT DEFINED ENV{ESP_MATTER_DEVICE_PATH} OR "$ENV{ESP_MATTER_DEVICE_PATH}" STREQUAL "" OR "$ENV{ESP_MATTER_DEVICE_PATH}" MATCHES "^\\$\\{env:")
    set(ENV{ESP_MATTER_DEVICE_PATH} $ENV{ESP_MATTER_PATH}/device_hal/device/esp32c6_devkit_c)
endif()

set(PROJECT_VER "1.0")
set(PROJECT_VER_NUMBER 1)

set(ESP_MATTER_PATH $ENV{ESP_MATTER_PATH})
set(MATTER_SDK_PATH ${ESP_MATTER_PATH}/connectedhomeip/connectedhomeip)

if(NOT DEFINED ENV{_PW_ACTUAL_ENVIRONMENT_ROOT} OR "$ENV{_PW_ACTUAL_ENVIRONMENT_ROOT}" STREQUAL "" OR "$ENV{_PW_ACTUAL_ENVIRONMENT_ROOT}" MATCHES "^\\$\\{env:")
    set(ENV{_PW_ACTUAL_ENVIRONMENT_ROOT} "${MATTER_SDK_PATH}/.environment")
endif()

if(NOT EXISTS "$ENV{_PW_ACTUAL_ENVIRONMENT_ROOT}/build_overrides/pigweed_environment.gni")
    message(FATAL_ERROR "Pigweed environment not found. Prepare the course esp-matter SDK environment first.")
endif()

set(_MATTER_COMMAND_ENV "\"${CMAKE_COMMAND}\" -E env")
foreach(_MATTER_ENV_VAR ESP_MATTER_PATH ESP_MATTER_DEVICE_PATH _PW_ACTUAL_ENVIRONMENT_ROOT ZAP_INSTALL_PATH)
    if(DEFINED ENV{${_MATTER_ENV_VAR}})
        string(APPEND _MATTER_COMMAND_ENV " \"${_MATTER_ENV_VAR}=$ENV{${_MATTER_ENV_VAR}}\"")
    endif()
endforeach()
set_property(GLOBAL PROPERTY RULE_LAUNCH_CUSTOM "${_MATTER_COMMAND_ENV}")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
include(${ESP_MATTER_PATH}/examples/common/cmake_common/components_include.cmake)
include($ENV{ESP_MATTER_DEVICE_PATH}/esp_matter_device.cmake)

set(EXTRA_COMPONENT_DIRS
    "${ESP_MATTER_PATH}/examples/common"
    "${MATTER_SDK_PATH}/config/esp32/components"
    "${ESP_MATTER_PATH}/components"
    "${ESP_MATTER_PATH}/device_hal/device"
    ${extra_components_dirs_append})

project(esp32c6_matter_light)

if(CONFIG_ENABLE_ENCRYPTED_OTA)
    target_add_binary_data(${CMAKE_PROJECT_NAME}.elf "esp_image_encryption_key.pem" TEXT)
endif()

idf_build_set_property(CXX_COMPILE_OPTIONS "-std=gnu++17;-Os;-DCHIP_HAVE_CONFIG_H;-Wno-overloaded-virtual" APPEND)
idf_build_set_property(C_COMPILE_OPTIONS "-Os" APPEND)
idf_build_set_property(COMPILE_OPTIONS "-Wno-format-nonliteral;-Wno-format-security" APPEND)
