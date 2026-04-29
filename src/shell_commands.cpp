/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <app/server/Server.h>

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/settings/settings.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/reboot.h>

static int cmd_decom(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(shell, "Factory reset requested (decommission).");
	chip::Server::GetInstance().ScheduleFactoryReset();
	return 0;
}

SHELL_CMD_REGISTER(decom, NULL, "Decommission device (factory reset).", cmd_decom);

static int cmd_wipe_settings(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	const struct flash_area *fa = nullptr;
	int err = flash_area_open(FIXED_PARTITION_ID(storage_partition), &fa);
	if (err != 0 || fa == nullptr) {
		shell_error(shell, "Failed to open storage partition: %d", err);
		return err != 0 ? err : -ENODEV;
	}

	err = flash_area_erase(fa, 0, fa->fa_size);
	flash_area_close(fa);
	if (err != 0) {
		shell_error(shell, "Failed to erase storage partition: %d", err);
		return err;
	}

	shell_print(shell, "Storage partition erased. Rebooting...");
	k_msleep(100);
	sys_reboot(SYS_REBOOT_COLD);
	return 0;
}

SHELL_CMD_REGISTER(wipe_settings, NULL, "Erase settings storage partition and reboot.", cmd_wipe_settings);

static int cmd_diag_dump(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	struct Ctx {
		const struct shell *shell;
		bool any;
	} ctx = {shell, false};

	int err = settings_load_subtree_direct(
		"tinyenv/diag",
		[](const char *key, size_t len, settings_read_cb read_cb, void *cb_arg, void *param) -> int {
			auto *ctx = static_cast<Ctx *>(param);
			uint32_t value = 0;
			if (len == sizeof(value) &&
			    read_cb(cb_arg, &value, sizeof(value)) == static_cast<ssize_t>(sizeof(value))) {
				shell_print(ctx->shell, "tinyenv/diag/%s: %u", key, value);
				ctx->any = true;
			}
			return 0;
		},
		&ctx);
	if (err != 0) {
		shell_error(shell, "diag read failed: %d", err);
		return err;
	}

	if (!ctx.any) {
		shell_print(shell, "No diagnostic keys found.");
	}

	return 0;
}

SHELL_CMD_REGISTER(diag_dump, NULL, "Dump TinyENV persisted diagnostic counters.", cmd_diag_dump);
