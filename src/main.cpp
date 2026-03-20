/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "app_task.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#if defined(CONFIG_USB_DEVICE_STACK) && !defined(CONFIG_USB_DEVICE_STACK_NEXT)
#include <zephyr/usb/usb_device.h>
#endif

LOG_MODULE_REGISTER(app, CONFIG_CHIP_APP_LOG_LEVEL);

int main()
{
#if defined(CONFIG_USB_DEVICE_STACK) && !defined(CONFIG_USB_DEVICE_STACK_NEXT)
	int usb_rc = usb_enable(NULL);
	if (usb_rc != 0) {
		LOG_ERR("USB enable failed: %d", usb_rc);
	}
#endif

	CHIP_ERROR err = AppTask::Instance().StartApp();

	LOG_ERR("Exited with code %" CHIP_ERROR_FORMAT, err.Format());
	return err == CHIP_NO_ERROR ? EXIT_SUCCESS : EXIT_FAILURE;
}
