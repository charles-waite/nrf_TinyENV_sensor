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
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <hal/nrf_saadc.h>
#include <zephyr/dt-bindings/adc/nrf-saadc.h>
#include <zephyr/devicetree.h>

#include <atomic>
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
#ifndef TINYENV_STATUS_LED_ENABLED
#define TINYENV_STATUS_LED_ENABLED 1
#endif

constexpr chip::EndpointId kTemperatureSensorEndpointId = 1;
constexpr uint32_t kLedHeartbeatIntervalMs = 5000;
constexpr uint32_t kLedHeartbeatOnMs = 200;
constexpr bool kEnableHeartbeat = false;
constexpr bool kEnableSleepLogs = false;
constexpr bool kEnableSensorLogs = false;

constexpr float kPlaceholderBatteryVolts = 3.69f;
constexpr float kBatteryGain = 1.0f; /* calibration placeholder */
constexpr float kVbatDividerRatio = (1000000.0f + 510000.0f) / 510000.0f; /* 1M/510k */
constexpr float kAdcRefVolts = 0.6f;
constexpr float kAdcGainVal = 1.0f / 6.0f;
constexpr uint8_t kAdcResolution = 12;
constexpr uint8_t kVbatEnablePin = 14; /* P0.14 */
constexpr uint8_t kVbatAdcChannelId = 0;
constexpr uint8_t kVbatAdcInput = NRF_SAADC_AIN7; /* P0.31 / AIN7 */

Nrf::Matter::IdentifyCluster sIdentifyCluster(kTemperatureSensorEndpointId);

#ifdef CONFIG_CHIP_ICD_UAT_SUPPORT
#define UAT_BUTTON_MASK DK_BTN3_MSK
#endif

const struct device *sAdcDev = DEVICE_DT_GET(DT_NODELABEL(adc));
const struct device *sGpioDev = DEVICE_DT_GET(DT_NODELABEL(gpio0));
const struct device *sI2c1Dev = DEVICE_DT_GET_OR_NULL(DT_NODELABEL(i2c1));
const struct device *sI2c0Dev = DEVICE_DT_GET_OR_NULL(DT_NODELABEL(i2c0));

#define LED0_NODE DT_ALIAS(led0)
#define LED1_NODE DT_ALIAS(led1)
#define LED2_NODE DT_ALIAS(led2)

#if DT_NODE_HAS_STATUS(LED0_NODE, okay)
static const struct gpio_dt_spec sLedRed = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
#endif
#if DT_NODE_HAS_STATUS(LED1_NODE, okay)
static const struct gpio_dt_spec sLedGreen = GPIO_DT_SPEC_GET(LED1_NODE, gpios);
#endif
#if DT_NODE_HAS_STATUS(LED2_NODE, okay)
static const struct gpio_dt_spec sLedBlue = GPIO_DT_SPEC_GET(LED2_NODE, gpios);
#endif

#if DT_NODE_HAS_STATUS(LED0_NODE, okay) || DT_NODE_HAS_STATUS(LED1_NODE, okay) || \
	DT_NODE_HAS_STATUS(LED2_NODE, okay)
static constexpr bool kStatusLedEnabled = (TINYENV_STATUS_LED_ENABLED != 0);
K_THREAD_STACK_DEFINE(sLedStack, 512);
static struct k_thread sLedThread;
static std::atomic<bool> sPulseActive{false};

#if DT_HAS_ALIAS(wake_btn)
static const struct gpio_dt_spec sWakeBtn = GPIO_DT_SPEC_GET(DT_ALIAS(wake_btn), gpios);
static struct gpio_callback sWakeBtnCb;
#endif

// XIAO nRF52840 RGB LED is active-high (1 = on, 0 = off).
static inline void SetLedState(const struct gpio_dt_spec & led, bool on)
{
	if (!device_is_ready(led.port)) {
		return;
	}
	gpio_pin_set_dt(&led, on ? 1 : 0);
}

static void SetLeds(bool redOn, bool greenOn, bool blueOn)
{
#if DT_NODE_HAS_STATUS(LED0_NODE, okay)
	SetLedState(sLedRed, redOn);
#endif
#if DT_NODE_HAS_STATUS(LED1_NODE, okay)
	SetLedState(sLedGreen, greenOn);
#endif
#if DT_NODE_HAS_STATUS(LED2_NODE, okay)
	SetLedState(sLedBlue, blueOn);
#endif
}

static void LedStatusThread(void *, void *, void *)
{
	uint32_t elapsed = 0;

	while (true) {
		if (!kStatusLedEnabled) {
			SetLeds(false, false, false);
			k_sleep(K_MSEC(500));
			continue;
		}

		if (sPulseActive.load()) {
			k_sleep(K_MSEC(50));
			continue;
		}

		const bool commissioned = chip::Server::GetInstance().GetFabricTable().FabricCount() > 0;
		const bool commissioning =
			chip::Server::GetInstance().GetCommissioningWindowManager().IsCommissioningWindowOpen();

		if (commissioning) {
			SetLeds(false, false, true);
		} else if (!commissioned) {
			SetLeds(true, false, false);
		} else {
			if (kEnableHeartbeat) {
				const bool heartbeatOn = elapsed < kLedHeartbeatOnMs;
				SetLeds(false, heartbeatOn, false);
				elapsed = (elapsed + 200) % kLedHeartbeatIntervalMs;
			} else {
				SetLeds(false, false, false);
			}
		}

		k_sleep(K_MSEC(200));
	}
}
#endif

static void LogI2CBusScan()
{
	struct Bus {
		const char *label;
		const struct device *dev;
	};

	const Bus buses[] = {
		{"I2C1", sI2c1Dev},
		{"I2C0", sI2c0Dev},
	};

	for (const auto &bus : buses) {
		if (!bus.dev || !device_is_ready(bus.dev)) {
			LOG_ERR("%s not ready", bus.label);
			continue;
		}

		uint8_t found_count = 0;
		for (uint8_t addr = 0x03; addr < 0x78; ++addr) {
			const int err = i2c_write(bus.dev, nullptr, 0, addr);
			if (err == 0) {
				LOG_WRN("%s device detected at 0x%02X", bus.label, addr);
				++found_count;
			}
		}

		if (found_count == 0) {
			LOG_ERR("No I2C devices found on %s", bus.label);
		}
	}
}

static void PulseGreenOnce()
{
#if DT_NODE_HAS_STATUS(LED1_NODE, okay)
	sPulseActive.store(true);
	SetLedState(sLedGreen, true);
	k_sleep(K_MSEC(200));
	SetLedState(sLedGreen, false);
	sPulseActive.store(false);
#else
	k_sleep(K_MSEC(200));
#endif
}

static void WakeButtonHandler(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(cb);
	ARG_UNUSED(pins);

	if (kEnableSleepLogs) {
		LOG_WRN("Wake button pressed.");
	}
	Server::GetInstance().GetICDManager().OnNetworkActivity();
}

static void InitWakeButton()
{
#if DT_HAS_ALIAS(wake_btn)
	if (!device_is_ready(sWakeBtn.port)) {
		LOG_ERR("Wake button GPIO not ready");
		return;
	}

	int err = gpio_pin_configure_dt(&sWakeBtn, GPIO_INPUT);
	if (err) {
		LOG_ERR("Wake button config failed: %d", err);
		return;
	}

	err = gpio_pin_interrupt_configure_dt(&sWakeBtn, GPIO_INT_EDGE_TO_ACTIVE);
	if (err) {
		LOG_ERR("Wake button interrupt config failed: %d", err);
		return;
	}

	gpio_init_callback(&sWakeBtnCb, WakeButtonHandler, BIT(sWakeBtn.pin));
	gpio_add_callback(sWakeBtn.port, &sWakeBtnCb);
#endif
}
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
			if (kEnableSleepLogs) {
				LOG_WRN("Waking from sleep. Updating sensors...");
			}
			if (!AppTask::Instance().UpdateSensorReadings(false)) {
				return;
			}

			AppTask::Instance().UpdateMatterAttributes();
			PulseGreenOnce();
			if (kEnableSleepLogs) {
				LOG_WRN("Entering sleep for %u seconds.", kSensorUpdateIntervalMs / 1000);
			}
		},
		reinterpret_cast<intptr_t>(timer->user_data));
}

CHIP_ERROR AppTask::Init()
{
	/* Initialize Matter stack */
	ReturnErrorOnFailure(Nrf::Matter::PrepareServer());

#if DT_NODE_HAS_STATUS(LED0_NODE, okay) || DT_NODE_HAS_STATUS(LED1_NODE, okay) || \
	DT_NODE_HAS_STATUS(LED2_NODE, okay)
	if (kStatusLedEnabled) {
#if DT_NODE_HAS_STATUS(LED0_NODE, okay)
		if (device_is_ready(sLedRed.port)) {
			(void)gpio_pin_configure_dt(&sLedRed, GPIO_OUTPUT_INACTIVE);
		}
#endif
#if DT_NODE_HAS_STATUS(LED1_NODE, okay)
		if (device_is_ready(sLedGreen.port)) {
			(void)gpio_pin_configure_dt(&sLedGreen, GPIO_OUTPUT_INACTIVE);
		}
#endif
#if DT_NODE_HAS_STATUS(LED2_NODE, okay)
		if (device_is_ready(sLedBlue.port)) {
			(void)gpio_pin_configure_dt(&sLedBlue, GPIO_OUTPUT_INACTIVE);
		}
#endif
		k_thread_create(&sLedThread, sLedStack, K_THREAD_STACK_SIZEOF(sLedStack),
				LedStatusThread, NULL, NULL, NULL, 5, 0, K_NO_WAIT);
	}
#endif

	if (!IS_ENABLED(CONFIG_TINYENV_DISABLE_BOARD_UI)) {
		if (!Nrf::GetBoard().Init(ButtonEventHandler)) {
			LOG_ERR("User interface initialization failed.");
			return CHIP_ERROR_INCORRECT_STATE;
		}

		/* Register Matter event handler that controls the connectivity status LED based on the captured Matter
		 * network state. */
		ReturnErrorOnFailure(Nrf::Matter::RegisterEventHandler(Nrf::Board::DefaultMatterEventHandler, 0));
	}

	ReturnErrorOnFailure(sIdentifyCluster.Init());

	mSht4x = DEVICE_DT_GET_ANY(sensirion_sht4x);
	if (mSht4x && device_is_ready(mSht4x)) {
		mShtReady = true;
		LOG_WRN("SHT4x sensor ready: %s", mSht4x->name);
	} else {
		mShtReady = false;
		LOG_ERR("SHT4x sensor not ready");
	}
	LogI2CBusScan();
	InitWakeButton();

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

	if (UpdateSensorReadings(true)) {
		UpdateMatterAttributes();
		PulseGreenOnce();
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
	/* Placeholder used if ADC init/read fails. */
	const float vbat = kPlaceholderBatteryVolts;
	return vbat * kBatteryGain;
}

static bool InitBatterySense()
{
	static bool sInit = false;
	static bool sReady = false;

	if (sInit) {
		return sReady;
	}
	sInit = true;

	if (!device_is_ready(sAdcDev)) {
		LOG_ERR("ADC not ready");
		return false;
	}
	if (!device_is_ready(sGpioDev)) {
		LOG_ERR("GPIO not ready");
		return false;
	}

	int err = gpio_pin_configure(sGpioDev, kVbatEnablePin, GPIO_OUTPUT_INACTIVE);
	if (err) {
		LOG_ERR("VBAT enable pin config failed: %d", err);
		return false;
	}
	/* VBAT sense enable is active-low (sink). */
	gpio_pin_set(sGpioDev, kVbatEnablePin, 1);

	adc_channel_cfg channel_cfg = {};
	channel_cfg.gain = ADC_GAIN_1_6;
	channel_cfg.reference = ADC_REF_INTERNAL;
	channel_cfg.acquisition_time = ADC_ACQ_TIME_DEFAULT;
	channel_cfg.channel_id = kVbatAdcChannelId;
	channel_cfg.input_positive = kVbatAdcInput;

	err = adc_channel_setup(sAdcDev, &channel_cfg);
	if (err) {
		LOG_ERR("ADC channel setup failed: %d", err);
		return false;
	}

	sReady = true;
	return true;
}

static float ReadBatteryVolts()
{
	if (!InitBatterySense()) {
		return ReadBatteryVoltsPlaceholder();
	}

	int16_t raw = 0;
	adc_sequence sequence = {};
	sequence.channels = BIT(kVbatAdcChannelId);
	sequence.buffer = &raw;
	sequence.buffer_size = sizeof(raw);
	sequence.resolution = kAdcResolution;

	gpio_pin_set(sGpioDev, kVbatEnablePin, 0);
	k_sleep(K_MSEC(2));
	const int err = adc_read(sAdcDev, &sequence);
	gpio_pin_set(sGpioDev, kVbatEnablePin, 1);

	if (err) {
		LOG_ERR("ADC read failed: %d", err);
		return ReadBatteryVoltsPlaceholder();
	}

	if (raw < 0) {
		raw = 0;
	}

	const float full_scale = kAdcRefVolts / kAdcGainVal;
	const float v_in = (static_cast<float>(raw) / ((1 << kAdcResolution) - 1)) * full_scale;
	const float vbat = v_in * kVbatDividerRatio * kBatteryGain;

	return vbat;
}

bool AppTask::UpdateSensorReadings(bool force)
{
	const bool commissioned = IsCommissioned();
	const int64_t now_ms = k_uptime_get();

	if (commissioned != mWasCommissioned) {
		mWasCommissioned = commissioned;
		mCommissionedAtMs = commissioned ? now_ms : 0;
	}

	if (!force && !commissioned) {
		return false;
	}

	if (!force && mCommissionedAtMs > 0 && (now_ms - mCommissionedAtMs) < kCommissionGraceMs) {
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

	static uint32_t sShtLogCount = 0;
	++sShtLogCount;
	if (kEnableSensorLogs && (sShtLogCount == 1 || (sShtLogCount % 10) == 0)) {
		LOG_WRN("SHT4x sample: %.2f C, %.2f %%RH", temp_c, rh);
	}

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

	const float vbat = ReadBatteryVolts();
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

	static uint32_t sBatteryLogCount = 0;
	++sBatteryLogCount;
	if (kEnableSensorLogs && (sBatteryLogCount == 1 || (sBatteryLogCount % 10) == 0)) {
		LOG_WRN("Battery sample: %.3f V (%u%%)", static_cast<double>(vbat), pct);
	}

	VLOG_DBG("Sensor update: temp=%d, rh=%u, vbat=%.3fV (%u%%)",
		 GetCurrentTemperature(), GetCurrentHumidity(), vbat, pct);
}
