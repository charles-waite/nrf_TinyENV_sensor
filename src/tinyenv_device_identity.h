/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include <lib/core/CHIPError.h>

CHIP_ERROR TinyEnvConfigureRuntimeMatterIdentity();
CHIP_ERROR TinyEnvInstallRuntimeDeviceInstanceInfoProvider();
CHIP_ERROR TinyEnvGetRuntimeSerialNumber(char *buf, size_t bufSize);
