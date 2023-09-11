/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2023 Ventana Micro Systems Inc.
 *
 * Authors:
 *   Subrahmanya Lingappa <slingappa@ventanamicro.com>
 */

#include <libfdt.h>
#include <sbi/riscv_io.h>
#include <sbi/sbi_ras.h>
#include <sbi/sbi_ecall_interface.h>
#include <sbi/sbi_scratch.h>
#include <sbi_utils/ras/fdt_ras.h>
#include <sbi_utils/fdt/fdt_helper.h>
#include <sbi_utils/mailbox/fdt_mailbox.h>
#include <sbi_utils/mailbox/rpmi_mailbox.h>

struct rpmi_ras {
	struct mbox_chan *chan;
};

static unsigned long rpmi_ras_offset;

static struct rpmi_ras *rpmi_ras_get_pointer(u32 hartid)
{
	struct sbi_scratch *scratch;

	scratch = sbi_hartid_to_scratch(hartid);
	if (!scratch || !rpmi_ras_offset)
		return NULL;

	return sbi_scratch_offset_ptr(scratch, rpmi_ras_offset);
}

static int rpmi_ras_sync_hart_errs(u32 *pending_vectors)
{
	int rc = SBI_SUCCESS;
	struct rpmi_ras_sync_hart_err_req req;
	struct rpmi_ras_sync_err_resp resp;
	struct rpmi_ras *ras;

	req.hart_id = current_hartid();

	ras = rpmi_ras_get_pointer(req.hart_id);

	rc = rpmi_normal_request_with_status(
			ras->chan, RPMI_RAS_SRV_SYNC_HART_ERR_REQ,
			&req, rpmi_u32_count(req), rpmi_u32_count(req),
			&resp, rpmi_u32_count(resp), rpmi_u32_count(resp));

	return rc;
}

static int rpmi_ras_sync_dev_errs(u32 *pending_vectors)
{
	int rc = SBI_SUCCESS;

	return rc;
}

static int rpmi_ras_probe(void)
{
	int rc;
	struct rpmi_ras *ras;
	struct rpmi_ras_probe_resp resp;
	struct rpmi_ras_probe_req req;
	u32 hart_id = current_hartid();

	ras = rpmi_ras_get_pointer(hart_id);
	if (!ras)
		return SBI_ENOSYS;

	rc = rpmi_normal_request_with_status(
			ras->chan, RPMI_RAS_SRV_PROBE_REQ,
			&req, rpmi_u32_count(req), rpmi_u32_count(req),
			&resp, rpmi_u32_count(resp), rpmi_u32_count(resp));
	if (rc)
		return rc;

	return 0;
}

static struct sbi_ras_agent sbi_rpmi_ras_agent = {
	.name			= "rpmi-ras-agent",
	.ras_sync_hart_errs	= rpmi_ras_sync_hart_errs,
	.ras_sync_dev_errs	= rpmi_ras_sync_dev_errs,
	.ras_probe		= rpmi_ras_probe,
};

static int rpmi_ras_cold_init(void *fdt, int nodeoff,
			      const struct fdt_match *match)
{
	int rc;
	struct mbox_chan *chan;

	if (!rpmi_ras_offset) {
		rpmi_ras_offset =
			sbi_scratch_alloc_type_offset(struct rpmi_ras);
		if (!rpmi_ras_offset)
			return SBI_ENOMEM;
	}

	/*
	 * If channel request failed then other end does not support
	 * RAS service group so do nothing.
	 */
	rc = fdt_mailbox_request_chan(fdt, nodeoff, 0, &chan);
	if (rc)
		return 0;

	sbi_ras_set_agent(&sbi_rpmi_ras_agent);

	return 0;
}

static const struct fdt_match rpmi_ras_match[] = {
	{ .compatible = "riscv,rpmi-ras" },
	{},
};

struct fdt_ras fdt_ras_rpmi = {
	.match_table = rpmi_ras_match,
	.cold_init = rpmi_ras_cold_init,
};
