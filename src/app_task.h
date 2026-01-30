/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "board/board.h"

#include <platform/CHIPDeviceLayer.h>

struct Identify;
struct device;

class AppTask {
public:
	static AppTask &Instance()
	{
		static AppTask sAppTask;
		return sAppTask;
	};

	CHIP_ERROR StartApp();

	int16_t GetCurrentTemperature() const { return mCurrentTemperature; }
	uint16_t GetCurrentHumidity() const { return mCurrentHumidity; }

private:
	CHIP_ERROR Init();
	k_timer mTimer;

	static constexpr uint32_t kSensorUpdateIntervalMs = 120000; /* 120 seconds */
	static constexpr uint32_t kCommissionGraceMs = 60000; /* allow initial networking */

	static void UpdateTemperatureTimeoutCallback(k_timer *timer);

	static void ButtonEventHandler(Nrf::ButtonState state, Nrf::ButtonMask hasChanged);

	bool UpdateSensorReadings();
	void UpdateMatterAttributes();
	bool IsCommissioned() const;

	const struct device *mSht4x = nullptr;
	bool mShtReady = false;
	int64_t mCommissionedAtMs = 0;
	bool mWasCommissioned = false;

	int16_t mTemperatureSensorMaxValue = 0;
	int16_t mTemperatureSensorMinValue = 0;
	int16_t mCurrentTemperature = 0;
	uint16_t mCurrentHumidity = 0;
};
