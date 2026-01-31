/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <app/server/Server.h>
#include <zephyr/shell/shell.h>

static int cmd_decom(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(shell, "Factory reset requested (decommission).");
	chip::Server::GetInstance().ScheduleFactoryReset();
	return 0;
}

SHELL_CMD_REGISTER(decom, NULL, "Decommission device (factory reset).", cmd_decom);
