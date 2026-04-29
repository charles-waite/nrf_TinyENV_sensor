/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#pragma once

#include "board/board.h"

#include <platform/CHIPDeviceLayer.h>
#include <openthread/thread.h>
#include <stdint.h>

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
	void MaybeLogAndPersistHealthSnapshot();
	void DiagOnBoot();
	void DiagOnThreadDetached();
	void DiagOnThreadReattached();
	void DiagOnSensorReadFailure();
	void DiagOnAdcReadFailure();
	void DiagFeedWatchdog();
	void DiagInitWatchdog();

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
	static void ThreadStateChangedCallback(otChangedFlags flags, void *context);

	static void ButtonEventHandler(Nrf::ButtonState state, Nrf::ButtonMask hasChanged);

	bool UpdateSensorReadings(bool force);
	void UpdateCommissioningAwakePolicy();
	void ApplyThreadTxPower(int8_t txPowerDbm, const char *context);
	void UpdateMatterAttributes();
	bool IsCommissioned() const;
	void HandleThreadStateChange(otChangedFlags flags);

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
	otDeviceRole mLastThreadRole = OT_DEVICE_ROLE_DISABLED;
	uint16_t mLastParentRloc16 = 0;
	bool mHasLastParent = false;
	bool mThreadDetached = false;
	int64_t mThreadDetachedSinceMs = 0;
	int64_t mLastHealthSnapshotMs = 0;
	int mWdtChannel = -1;
	uint32_t mDiagBootCount = 0;
	uint32_t mDiagWdtResetCount = 0;
	uint32_t mDiagThreadDetachCount = 0;
	uint32_t mDiagThreadReattachCount = 0;
	uint32_t mDiagSensorFailureCount = 0;
	uint32_t mDiagAdcFailureCount = 0;
	uint32_t mDiagLastResetReas = 0;
};
