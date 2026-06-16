/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "app_task.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#if CONFIG_TINYENV_WIPE_SETTINGS_ON_BOOT
#include <zephyr/storage/flash_map.h>
#endif
#if defined(CONFIG_USB_DEVICE_STACK) && !defined(CONFIG_USB_DEVICE_STACK_NEXT)
#include <zephyr/usb/usb_device.h>
#endif

LOG_MODULE_REGISTER(app, CONFIG_CHIP_APP_LOG_LEVEL);

#if CONFIG_TINYENV_WIPE_SETTINGS_ON_BOOT
static void WipeSettingsAndPark()
{
	const struct flash_area *fa = nullptr;
	int err = flash_area_open(FIXED_PARTITION_ID(storage_partition), &fa);
	if (err != 0 || fa == nullptr) {
		LOG_ERR("Settings wipe: failed to open storage partition: %d", err);
	} else {
		err = flash_area_erase(fa, 0, fa->fa_size);
		flash_area_close(fa);
		if (err == 0) {
			LOG_ERR("Settings wipe: storage partition erased. Flash a normal UF2 image now.");
		} else {
			LOG_ERR("Settings wipe: erase failed: %d", err);
		}
	}

	while (true) {
		k_sleep(K_FOREVER);
	}
}
#endif

int main()
{
#if defined(CONFIG_USB_DEVICE_STACK) && !defined(CONFIG_USB_DEVICE_STACK_NEXT)
	int usb_rc = usb_enable(NULL);
	if (usb_rc != 0) {
		LOG_ERR("USB enable failed: %d", usb_rc);
	}
#endif

#if CONFIG_TINYENV_WIPE_SETTINGS_ON_BOOT
	WipeSettingsAndPark();
#endif

	CHIP_ERROR err = AppTask::Instance().StartApp();

	LOG_ERR("Exited with code %" CHIP_ERROR_FORMAT, err.Format());
	return err == CHIP_NO_ERROR ? EXIT_SUCCESS : EXIT_FAILURE;
}
