/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "tinyenv_device_identity.h"

#include <crypto/CHIPCryptoPAL.h>
#include <lib/support/CodeUtils.h>
#include <platform/ConfigurationManager.h>
#include <platform/DeviceInstanceInfoProvider.h>
#include <platform/nrfconnect/DeviceInstanceInfoProviderImpl.h>

#include <hal/nrf_ficr.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>

#include <cstdio>
#include <cstring>

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

namespace
{
constexpr size_t kMatterRuntimeIdentitySeedLen = 16;
constexpr size_t kMatterRotatingIdUniqueIdLen = 16;

void GetHardwareIdentitySeed(uint8_t *seed)
{
	sys_put_le32(nrf_ficr_deviceid_get(NRF_FICR, 0), &seed[0]);
	sys_put_le32(nrf_ficr_deviceid_get(NRF_FICR, 1), &seed[4]);
	sys_put_le32(nrf_ficr_deviceaddr_get(NRF_FICR, 0), &seed[8]);
	sys_put_le32(nrf_ficr_deviceaddr_get(NRF_FICR, 1), &seed[12]);
}

CHIP_ERROR GetRuntimeRotatingIdUniqueId(chip::MutableByteSpan &uniqueIdSpan)
{
	VerifyOrReturnError(uniqueIdSpan.size() >= kMatterRotatingIdUniqueIdLen, CHIP_ERROR_BUFFER_TOO_SMALL);

	uint8_t seed[kMatterRuntimeIdentitySeedLen];
	uint8_t digest[chip::Crypto::kSHA256_Hash_Length];
	GetHardwareIdentitySeed(seed);

	ReturnErrorOnFailure(chip::Crypto::Hash_SHA256(seed, sizeof(seed), digest));
	std::memcpy(uniqueIdSpan.data(), digest, kMatterRotatingIdUniqueIdLen);
	uniqueIdSpan.reduce_size(kMatterRotatingIdUniqueIdLen);

	return CHIP_NO_ERROR;
}

class TinyEnvDeviceInstanceInfoProvider final : public chip::DeviceLayer::DeviceInstanceInfoProviderImpl {
public:
	TinyEnvDeviceInstanceInfoProvider() :
		chip::DeviceLayer::DeviceInstanceInfoProviderImpl(
			chip::DeviceLayer::ConfigurationManagerImpl::GetDefaultInstance())
	{
	}

	CHIP_ERROR GetSerialNumber(char *buf, size_t bufSize) override
	{
		return TinyEnvGetRuntimeSerialNumber(buf, bufSize);
	}

	CHIP_ERROR GetRotatingDeviceIdUniqueId(chip::MutableByteSpan &uniqueIdSpan) override
	{
		return GetRuntimeRotatingIdUniqueId(uniqueIdSpan);
	}
};

TinyEnvDeviceInstanceInfoProvider &RuntimeDeviceInstanceInfoProvider()
{
	static TinyEnvDeviceInstanceInfoProvider provider;
	return provider;
}
} // namespace

CHIP_ERROR TinyEnvGetRuntimeSerialNumber(char *buf, size_t bufSize)
{
	const int written = std::snprintf(buf, bufSize, "TINYENV-NRF-%08lX%08lX",
					 static_cast<unsigned long>(nrf_ficr_deviceid_get(NRF_FICR, 1)),
					 static_cast<unsigned long>(nrf_ficr_deviceid_get(NRF_FICR, 0)));

	VerifyOrReturnError(written > 0, CHIP_ERROR_INTERNAL);
	VerifyOrReturnError(static_cast<size_t>(written) < bufSize, CHIP_ERROR_BUFFER_TOO_SMALL);

	return CHIP_NO_ERROR;
}

CHIP_ERROR TinyEnvInstallRuntimeDeviceInstanceInfoProvider()
{
	chip::DeviceLayer::SetDeviceInstanceInfoProvider(&RuntimeDeviceInstanceInfoProvider());
	return CHIP_NO_ERROR;
}

CHIP_ERROR TinyEnvConfigureRuntimeMatterIdentity()
{
	char serial[chip::DeviceLayer::ConfigurationManager::kMaxSerialNumberLength + 1];
	ReturnErrorOnFailure(TinyEnvGetRuntimeSerialNumber(serial, sizeof(serial)));
	ReturnErrorOnFailure(TinyEnvInstallRuntimeDeviceInstanceInfoProvider());
	LOG_WRN("Matter runtime identity: serial=%s", serial);

	return CHIP_NO_ERROR;
}
