#include "diot_commissioning.h"

#include <lib/support/logging/CHIPLogging.h>
#include <crypto/CHIPCryptoPAL.h>
#include <lib/support/CodeUtils.h>
#include <platform/CommissionableDataProvider.h>
#include <platform/DeviceInstanceInfoProvider.h>
#include <platform/ConnectivityManager.h>
#include <setup_payload/OnboardingCodesUtil.h>
#include <setup_payload/SetupPayload.h>

namespace {

constexpr const char *kTag = "diot_commissioning";
constexpr uint16_t kSetupDiscriminator = 3840;
#ifdef CONFIG_DIOT_MATTER_SETUP_PIN_CODE
constexpr uint32_t kSetupPinCode = CONFIG_DIOT_MATTER_SETUP_PIN_CODE;
#else
constexpr uint32_t kSetupPinCode = 20202021;
#endif
#ifdef CONFIG_DIOT_MATTER_BLE_DEVICE_NAME
constexpr char kBleDeviceName[] = CONFIG_DIOT_MATTER_BLE_DEVICE_NAME;
#else
constexpr char kBleDeviceName[] = "DIOT-C6";
#endif

static_assert(sizeof(kBleDeviceName) - 1 <= 31, "BLE device name must fit CHIP max length");

class fixed_commissionable_data_provider : public chip::DeviceLayer::CommissionableDataProvider {
public:
    CHIP_ERROR GetSetupDiscriminator(uint16_t &setupDiscriminator) override
    {
        setupDiscriminator = kSetupDiscriminator;
        return CHIP_NO_ERROR;
    }

    CHIP_ERROR SetSetupDiscriminator(uint16_t setupDiscriminator) override
    {
        (void)setupDiscriminator;
        return CHIP_ERROR_NOT_IMPLEMENTED;
    }

    CHIP_ERROR GetSpake2pIterationCount(uint32_t &iterationCount) override
    {
        iterationCount = kIterationCount;
        return CHIP_NO_ERROR;
    }

    CHIP_ERROR GetSpake2pSalt(chip::MutableByteSpan &saltBuf) override
    {
        VerifyOrReturnError(saltBuf.size() >= sizeof(kSalt), CHIP_ERROR_BUFFER_TOO_SMALL);
        memcpy(saltBuf.data(), kSalt, sizeof(kSalt));
        saltBuf.reduce_size(sizeof(kSalt));
        return CHIP_NO_ERROR;
    }

    CHIP_ERROR GetSpake2pVerifier(chip::MutableByteSpan &verifierBuf, size_t &outVerifierLen) override
    {
        uint32_t setupPasscode = 0;
        uint32_t iterationCount = 0;
        uint8_t saltStorage[sizeof(kSalt)] = {0};
        chip::MutableByteSpan saltSpan(saltStorage);

        ReturnErrorOnFailure(GetSetupPasscode(setupPasscode));
        ReturnErrorOnFailure(GetSpake2pIterationCount(iterationCount));
        ReturnErrorOnFailure(GetSpake2pSalt(saltSpan));

        chip::Crypto::Spake2pVerifier verifier;
        ReturnErrorOnFailure(verifier.Generate(iterationCount, saltSpan, setupPasscode));
        ReturnErrorOnFailure(verifier.Serialize(verifierBuf));

        outVerifierLen = verifierBuf.size();
        return CHIP_NO_ERROR;
    }

    CHIP_ERROR GetSetupPasscode(uint32_t &setupPasscode) override
    {
        VerifyOrReturnError(chip::SetupPayload::IsValidSetupPIN(kSetupPinCode), CHIP_ERROR_INVALID_ARGUMENT);
        setupPasscode = kSetupPinCode;
        return CHIP_NO_ERROR;
    }

    CHIP_ERROR SetSetupPasscode(uint32_t setupPasscode) override
    {
        (void)setupPasscode;
        return CHIP_ERROR_NOT_IMPLEMENTED;
    }

private:
    static constexpr uint32_t kIterationCount = 10000;
    static constexpr uint8_t kSalt[16] = {
        0x73, 0x61, 0x6c, 0x74, 0x2d, 0x64, 0x69, 0x6f,
        0x74, 0x2d, 0x63, 0x36, 0x2d, 0x30, 0x31, 0x00,
    };
};

// Lab-only values so onboarding QR generation never aborts when factory data is absent.
// If you later move to factory data, replace this provider with ESP32 factory provisioning.
class fixed_device_instance_info_provider : public chip::DeviceLayer::DeviceInstanceInfoProvider {
public:
    CHIP_ERROR GetVendorName(char * buf, size_t bufSize) override
    {
        constexpr char kVendorName[] = "DIOT";
        VerifyOrReturnError(bufSize >= sizeof(kVendorName), CHIP_ERROR_BUFFER_TOO_SMALL);
        memcpy(buf, kVendorName, sizeof(kVendorName));
        return CHIP_NO_ERROR;
    }

    CHIP_ERROR GetVendorId(uint16_t & vendorId) override
    {
        vendorId = 0xFFF1;
        return CHIP_NO_ERROR;
    }

    CHIP_ERROR GetProductName(char * buf, size_t bufSize) override
    {
        constexpr char kProductName[] = "ESP32C6 Matter Light";
        VerifyOrReturnError(bufSize >= sizeof(kProductName), CHIP_ERROR_BUFFER_TOO_SMALL);
        memcpy(buf, kProductName, sizeof(kProductName));
        return CHIP_NO_ERROR;
    }

    CHIP_ERROR GetProductId(uint16_t & productId) override
    {
        productId = 0x8000;
        return CHIP_NO_ERROR;
    }

    CHIP_ERROR GetPartNumber(char * buf, size_t bufSize) override
    {
        (void)buf;
        (void)bufSize;
        return CHIP_ERROR_NOT_IMPLEMENTED;
    }

    CHIP_ERROR GetProductURL(char * buf, size_t bufSize) override
    {
        (void)buf;
        (void)bufSize;
        return CHIP_ERROR_NOT_IMPLEMENTED;
    }

    CHIP_ERROR GetProductLabel(char * buf, size_t bufSize) override
    {
        (void)buf;
        (void)bufSize;
        return CHIP_ERROR_NOT_IMPLEMENTED;
    }

    CHIP_ERROR GetSerialNumber(char * buf, size_t bufSize) override
    {
        constexpr char kSerialNumber[] = "DIOT-LIGHT-01";
        VerifyOrReturnError(bufSize >= sizeof(kSerialNumber), CHIP_ERROR_BUFFER_TOO_SMALL);
        memcpy(buf, kSerialNumber, sizeof(kSerialNumber));
        return CHIP_NO_ERROR;
    }

    CHIP_ERROR GetManufacturingDate(uint16_t & year, uint8_t & month, uint8_t & day) override
    {
        year = 2026;
        month = 7;
        day = 23;
        return CHIP_NO_ERROR;
    }

    CHIP_ERROR GetHardwareVersion(uint16_t & hardwareVersion) override
    {
        hardwareVersion = 1;
        return CHIP_NO_ERROR;
    }

    CHIP_ERROR GetHardwareVersionString(char * buf, size_t bufSize) override
    {
        constexpr char kHwVersion[] = "1.0";
        VerifyOrReturnError(bufSize >= sizeof(kHwVersion), CHIP_ERROR_BUFFER_TOO_SMALL);
        memcpy(buf, kHwVersion, sizeof(kHwVersion));
        return CHIP_NO_ERROR;
    }

    CHIP_ERROR GetRotatingDeviceIdUniqueId(chip::MutableByteSpan & uniqueIdSpan) override
    {
        constexpr uint8_t kUniqueId[] = { 0x44, 0x49, 0x4F, 0x54, 0x2D, 0x4C, 0x49, 0x47,
                                          0x48, 0x54, 0x2D, 0x30, 0x31, 0x00, 0x00, 0x01 };
        VerifyOrReturnError(uniqueIdSpan.size() >= sizeof(kUniqueId), CHIP_ERROR_BUFFER_TOO_SMALL);
        memcpy(uniqueIdSpan.data(), kUniqueId, sizeof(kUniqueId));
        uniqueIdSpan.reduce_size(sizeof(kUniqueId));
        return CHIP_NO_ERROR;
    }
};

fixed_commissionable_data_provider sProvider;
fixed_device_instance_info_provider sDeviceInfoProvider;

} // namespace

namespace diot::commissioning {

CHIP_ERROR configure_commissioning()
{
    uint32_t setupPin = 0;
    ReturnErrorOnFailure(sProvider.GetSetupPasscode(setupPin));
    chip::DeviceLayer::SetCommissionableDataProvider(&sProvider);
    chip::DeviceLayer::SetDeviceInstanceInfoProvider(&sDeviceInfoProvider);
    ChipLogProgress(AppServer, "[%s] Setup PIN code: %u", kTag, static_cast<unsigned>(setupPin));
    ChipLogProgress(AppServer, "[%s] Setup discriminator: %u", kTag, static_cast<unsigned>(kSetupDiscriminator));
    PrintOnboardingCodes(chip::RendezvousInformationFlags(chip::RendezvousInformationFlag::kBLE));
    return CHIP_NO_ERROR;
}

CHIP_ERROR configure_ble_name()
{
#if CHIP_DEVICE_CONFIG_ENABLE_CHIPOBLE
    CHIP_ERROR err = chip::DeviceLayer::ConnectivityMgr().SetBLEDeviceName(kBleDeviceName);
    if (err == CHIP_NO_ERROR) {
        ChipLogProgress(AppServer, "[%s] BLE device name: %s", kTag, kBleDeviceName);
    }
    return err;
#else
    (void)kBleDeviceName;
    return CHIP_ERROR_UNSUPPORTED_CHIP_FEATURE;
#endif
}

} // namespace diot::commissioning
