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
	k_timer mCommissionPolicyTimer;

	static constexpr uint32_t kSensorUpdateIntervalMs = 60000; /* 60 seconds */
	static constexpr uint32_t kCommissionGraceMs = 30000; /* allow initial networking */
	static constexpr uint32_t kCommissionWindowPostCloseGraceMs = 180000; /* 3 minutes */
	static constexpr uint32_t kCommissionAwakeKickPeriodMs = 1000; /* keep ICD active while pairing */
	static constexpr int8_t kCommissioningThreadTxPowerDbm = 8; /* use full Thread TX power while pairing */
	static constexpr int8_t kPostCommissionThreadTxPowerDbm =
		CONFIG_TINYENV_POST_COMMISSION_THREAD_TX_POWER_DBM;

	static void UpdateTemperatureTimeoutCallback(k_timer *timer);
	static void CommissionPolicyTimeoutCallback(k_timer *timer);

	static void ButtonEventHandler(Nrf::ButtonState state, Nrf::ButtonMask hasChanged);

	bool UpdateSensorReadings(bool force);
	void UpdateCommissioningAwakePolicy();
	void ApplyThreadTxPower(int8_t txPowerDbm, const char *context);
	void UpdateMatterAttributes();
	bool IsCommissioned() const;

	const struct device *mSht4x = nullptr;
	bool mShtReady = false;
	int64_t mCommissionedAtMs = 0;
	bool mWasCommissioned = false;
	bool mCommissioningWindowOpen = false;
	int64_t mStayAwakeUntilMs = 0;
	int64_t mLastAwakeKickMs = 0;
	int8_t mAppliedThreadTxPowerDbm = -128;

	int16_t mTemperatureSensorMaxValue = 0;
	int16_t mTemperatureSensorMinValue = 0;
	int16_t mCurrentTemperature = 0;
	uint16_t mCurrentHumidity = 0;
};
