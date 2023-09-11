/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2023 Ventana Micro Systems Inc.
 *
 */

#include <sbi/sbi_error.h>
#include <sbi/sbi_ras.h>

static const struct sbi_ras_agent *ras_agent = NULL;

const struct sbi_ras_agent *sbi_ras_get_agent(void)
{
	return ras_agent;
}

void sbi_ras_set_agent(const struct sbi_ras_agent *agent)
{
	if (!agent || ras_agent)
		return;

	ras_agent = agent;
}

int sbi_ras_probe(void)
{
	if (!ras_agent || !ras_agent->ras_probe)
		return SBI_EFAIL;

	return ras_agent->ras_probe();
}

int sbi_ras_sync_hart_errs(u32 *pending_vectors)
{
	if (!ras_agent)
		return SBI_EFAIL;

	return ras_agent->ras_sync_hart_errs(pending_vectors);
}

int sbi_ras_sync_dev_errs(u32 *pending_vectors)
{
	if (!ras_agent)
		return SBI_EFAIL;

	return ras_agent->ras_sync_dev_errs(pending_vectors);
}

