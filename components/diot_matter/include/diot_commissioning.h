#pragma once

#include <lib/core/CHIPError.h>

namespace diot::commissioning {

CHIP_ERROR configure_commissioning();
CHIP_ERROR configure_ble_name();

} // namespace diot::commissioning
