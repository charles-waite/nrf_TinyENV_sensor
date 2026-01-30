/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "app_task.h"

#include "app/matter_init.h"
#include "app/task_executor.h"
#include "board/board.h"
#include "clusters/identify.h"
#include "lib/core/CHIPError.h"

#include <app/server/Server.h>
#include <app-common/zap-generated/attributes/Accessors.h>
#include <lib/support/Span.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>

#include <cmath>

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

#if defined(CONFIG_TINYENV_VERBOSE_LOGS)
#define VLOG_DBG(...) LOG_DBG(__VA_ARGS__)
#else
#define VLOG_DBG(...)                                                                         \
	do {                                                                                   \
	} while (0)
#endif

using namespace ::chip;
using namespace ::chip::app;
using namespace ::chip::DeviceLayer;

namespace
{
constexpr chip::EndpointId kTemperatureSensorEndpointId = 1;

constexpr float kPlaceholderBatteryVolts = 3.69f;
constexpr float kBatteryGain = 1.0f; /* calibration placeholder */

Nrf::Matter::IdentifyCluster sIdentifyCluster(kTemperatureSensorEndpointId);

#ifdef CONFIG_CHIP_ICD_UAT_SUPPORT
#define UAT_BUTTON_MASK DK_BTN3_MSK
#endif
} /* namespace */

void AppTask::ButtonEventHandler(Nrf::ButtonState state, Nrf::ButtonMask hasChanged)
{
#ifdef CONFIG_CHIP_ICD_UAT_SUPPORT
	if ((UAT_BUTTON_MASK & state & hasChanged)) {
		LOG_INF("ICD UserActiveMode has been triggered.");
		Server::GetInstance().GetICDManager().OnNetworkActivity();
	}
#endif
}

void AppTask::UpdateTemperatureTimeoutCallback(k_timer *timer)
{
	if (!timer || !timer->user_data) {
		return;
	}

	DeviceLayer::PlatformMgr().ScheduleWork(
		[](intptr_t p) {
			if (!AppTask::Instance().UpdateSensorReadings()) {
				return;
			}

			AppTask::Instance().UpdateMatterAttributes();
		},
		reinterpret_cast<intptr_t>(timer->user_data));
}

CHIP_ERROR AppTask::Init()
{
	/* Initialize Matter stack */
	ReturnErrorOnFailure(Nrf::Matter::PrepareServer());

	if (!Nrf::GetBoard().Init(ButtonEventHandler)) {
		LOG_ERR("User interface initialization failed.");
		return CHIP_ERROR_INCORRECT_STATE;
	}

	/* Register Matter event handler that controls the connectivity status LED based on the captured Matter network
	 * state. */
	ReturnErrorOnFailure(Nrf::Matter::RegisterEventHandler(Nrf::Board::DefaultMatterEventHandler, 0));

	ReturnErrorOnFailure(sIdentifyCluster.Init());

	mSht4x = DEVICE_DT_GET_ANY(sensirion_sht4x);
	if (mSht4x && device_is_ready(mSht4x)) {
		mShtReady = true;
		LOG_INF("SHT4x sensor ready");
	} else {
		mShtReady = false;
		LOG_ERR("SHT4x sensor not ready");
	}

#ifdef CONFIG_CHIP_ICD_UAT_SUPPORT
	{
		using IcdHint = Clusters::IcdManagement::UserActiveModeTriggerBitmap;
		chip::BitMask<IcdHint> hint;
		hint.Set(IcdHint::kPowerCycle);
		hint.Set(IcdHint::kCustomInstruction);
		Clusters::IcdManagement::Attributes::UserActiveModeTriggerHint::Set(chip::kRootEndpointId, hint);

		static constexpr char kUatInstruction[] = "Power cycle device to wake";
		Clusters::IcdManagement::Attributes::UserActiveModeTriggerInstruction::Set(
			chip::kRootEndpointId, CharSpan::fromCharString(kUatInstruction));
	}
#endif

	return Nrf::Matter::StartServer();
}

CHIP_ERROR AppTask::StartApp()
{
	ReturnErrorOnFailure(Init());

	DataModel::Nullable<int16_t> val;
	Protocols::InteractionModel::Status status =
		Clusters::TemperatureMeasurement::Attributes::MinMeasuredValue::Get(kTemperatureSensorEndpointId, val);

	if (status != Protocols::InteractionModel::Status::Success || val.IsNull()) {
		LOG_ERR("Failed to get temperature measurement min value %x", to_underlying(status));
		return CHIP_ERROR_INCORRECT_STATE;
	}

	static constexpr int16_t kSht4xMinCentiDegC = -4000;
	static constexpr int16_t kSht4xMaxCentiDegC = 12500;

	Clusters::TemperatureMeasurement::Attributes::MinMeasuredValue::Set(kTemperatureSensorEndpointId,
									   kSht4xMinCentiDegC);
	Clusters::TemperatureMeasurement::Attributes::MaxMeasuredValue::Set(kTemperatureSensorEndpointId,
									   kSht4xMaxCentiDegC);

	mTemperatureSensorMinValue = kSht4xMinCentiDegC;
	mTemperatureSensorMaxValue = kSht4xMaxCentiDegC;

	k_timer_init(&mTimer, AppTask::UpdateTemperatureTimeoutCallback, nullptr);
	k_timer_user_data_set(&mTimer, this);
	k_timer_start(&mTimer, K_MSEC(kSensorUpdateIntervalMs), K_MSEC(kSensorUpdateIntervalMs));

	if (UpdateSensorReadings()) {
		UpdateMatterAttributes();
	}

	while (true) {
		Nrf::DispatchNextTask();
	}

	return CHIP_NO_ERROR;
}

bool AppTask::IsCommissioned() const
{
	return Server::GetInstance().GetFabricTable().FabricCount() > 0;
}

static uint8_t VoltsToPercent(float v)
{
	if (v >= 4.14f)
		return 100;
	if (v <= 3.00f)
		return 0;

	struct Vp {
		float v;
		uint8_t p;
	};
	static constexpr Vp kCurve[] = {
		{4.14f, 100}, {4.10f, 95}, {4.00f, 85}, {3.90f, 75}, {3.80f, 65}, {3.70f, 50},
		{3.60f, 35},  {3.50f, 20}, {3.40f, 10}, {3.30f, 5},  {3.20f, 2},  {3.00f, 0},
	};

	for (size_t i = 0; i + 1 < (sizeof(kCurve) / sizeof(kCurve[0])); ++i) {
		const Vp hi = kCurve[i];
		const Vp lo = kCurve[i + 1];
		if (v <= hi.v && v >= lo.v) {
			const float t = (v - lo.v) / (hi.v - lo.v);
			const float p = lo.p + t * (hi.p - lo.p);
			return static_cast<uint8_t>(lroundf(p));
		}
	}
	return 0;
}

static float ReadBatteryVoltsPlaceholder()
{
	/* TODO: Replace with ADC-based VBAT read (likely 1:2 divider). */
	const float vbat = kPlaceholderBatteryVolts;
	return vbat * kBatteryGain;
}

bool AppTask::UpdateSensorReadings()
{
	const bool commissioned = IsCommissioned();
	const int64_t now_ms = k_uptime_get();

	if (commissioned != mWasCommissioned) {
		mWasCommissioned = commissioned;
		mCommissionedAtMs = commissioned ? now_ms : 0;
	}

	if (!commissioned) {
		return false;
	}

	if (mCommissionedAtMs > 0 && (now_ms - mCommissionedAtMs) < kCommissionGraceMs) {
		return false;
	}

	if (!mShtReady) {
		return false;
	}

	if (sensor_sample_fetch(mSht4x) != 0) {
		VLOG_DBG("SHT4x sample fetch failed");
		return false;
	}

	sensor_value temp_sv;
	sensor_value rh_sv;
	if (sensor_channel_get(mSht4x, SENSOR_CHAN_AMBIENT_TEMP, &temp_sv) != 0 ||
	    sensor_channel_get(mSht4x, SENSOR_CHAN_HUMIDITY, &rh_sv) != 0) {
		VLOG_DBG("SHT4x channel read failed");
		return false;
	}

	const double temp_c = sensor_value_to_double(&temp_sv);
	const double rh = sensor_value_to_double(&rh_sv);

	mCurrentTemperature = static_cast<int16_t>(lround(temp_c * 100.0));
	mCurrentHumidity = static_cast<uint16_t>(lround(rh * 100.0));

	return true;
}

void AppTask::UpdateMatterAttributes()
{
	Protocols::InteractionModel::Status status =
		Clusters::TemperatureMeasurement::Attributes::MeasuredValue::Set(
			kTemperatureSensorEndpointId, GetCurrentTemperature());
	if (status != Protocols::InteractionModel::Status::Success) {
		LOG_ERR("Updating temperature measurement failed %x", to_underlying(status));
	}

	status = Clusters::RelativeHumidityMeasurement::Attributes::MeasuredValue::Set(
		kTemperatureSensorEndpointId, GetCurrentHumidity());
	if (status != Protocols::InteractionModel::Status::Success) {
		LOG_ERR("Updating humidity measurement failed %x", to_underlying(status));
	}

	const float vbat = ReadBatteryVoltsPlaceholder();
	const uint32_t mv = static_cast<uint32_t>(lroundf(vbat * 1000.0f));
	const uint8_t pct = VoltsToPercent(vbat);
	const uint8_t half_pct = (pct >= 100) ? 200 : static_cast<uint8_t>(pct * 2);

	Clusters::PowerSource::Attributes::Status::Set(kTemperatureSensorEndpointId,
						       Clusters::PowerSource::PowerSourceStatusEnum::kActive);
	Clusters::PowerSource::Attributes::BatPresent::Set(kTemperatureSensorEndpointId, true);
	Clusters::PowerSource::Attributes::BatReplaceability::Set(
		kTemperatureSensorEndpointId, Clusters::PowerSource::BatReplaceabilityEnum::kUserReplaceable);
	Clusters::PowerSource::Attributes::BatChargeLevel::Set(
		kTemperatureSensorEndpointId, Clusters::PowerSource::BatChargeLevelEnum::kOk);
	Clusters::PowerSource::Attributes::BatVoltage::Set(kTemperatureSensorEndpointId, mv);
	Clusters::PowerSource::Attributes::BatPercentRemaining::Set(kTemperatureSensorEndpointId, half_pct);

	VLOG_DBG("Sensor update: temp=%d, rh=%u, vbat=%.3fV (%u%%)",
		 GetCurrentTemperature(), GetCurrentHumidity(), vbat, pct);
}
