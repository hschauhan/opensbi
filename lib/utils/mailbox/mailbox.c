/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2022 Ventana Micro Systems Inc.
 *
 * Authors:
 *   Anup Patel <apatel@ventanamicro.com>
 */

#include <sbi/sbi_error.h>
#include <sbi/sbi_string.h>
#include <sbi_utils/mailbox/mailbox.h>

static SBI_LIST_HEAD(mbox_list);

struct mbox_controller *mbox_controller_find(unsigned int id)
{
	struct sbi_dlist *pos;

	sbi_list_for_each(pos, &mbox_list) {
		struct mbox_controller *mbox = to_mbox_controller(pos);

		if (mbox->id == id)
			return mbox;
	}

	return NULL;
}

int mbox_controller_add(struct mbox_controller *mbox)
{
	if (!mbox)
		return SBI_EINVAL;
	if (mbox_controller_find(mbox->id))
		return SBI_EALREADY;

	SBI_INIT_LIST_HEAD(&mbox->node);
	ATOMIC_INIT(&mbox->xfer_next_seq, 0);
	SBI_INIT_LIST_HEAD(&mbox->chan_list);
	sbi_list_add(&mbox->node, &mbox_list);

	return 0;
}

void mbox_controller_remove(struct mbox_controller *mbox)
{
	struct mbox_chan *chan;

	if (!mbox)
		return;

	while (!sbi_list_empty(&mbox->chan_list)) {
		chan = sbi_list_first_entry(&mbox->chan_list,
					    struct mbox_chan, node);
		if (mbox->free_chan)
			mbox->free_chan(mbox, chan);
		sbi_list_del(&chan->node);
	}

	sbi_list_del(&mbox->node);
}

struct mbox_chan *mbox_controller_request_chan(struct mbox_controller *mbox,
					       u32 *chan_args)
{
	struct mbox_chan *ret;
	struct sbi_dlist *pos;

	if (!chan_args || !mbox || !mbox->request_chan)
		return NULL;

	sbi_list_for_each(pos, &mbox->chan_list) {
		ret = to_mbox_chan(pos);
		if (!sbi_memcmp(ret->chan_args, chan_args,
				sizeof(ret->chan_args)))
			return ret;
	}

	ret = mbox->request_chan(mbox, chan_args);
	if (!ret)
		return NULL;

	SBI_INIT_LIST_HEAD(&ret->node);
	ret->mbox = mbox;
	sbi_memcpy(ret->chan_args, chan_args, sizeof(ret->chan_args));
	sbi_list_add(&ret->node, &mbox->chan_list);
	return ret;
}

void mbox_controller_free_chan(struct mbox_chan *chan)
{
	if (!chan || !chan->mbox)
		return;

	if (chan->mbox->free_chan)
		chan->mbox->free_chan(chan->mbox, chan);
	sbi_list_del(&chan->node);
}

int mbox_chan_xfer(struct mbox_chan *chan, struct mbox_xfer *xfer)
{
	if (!xfer || !chan || !chan->mbox || !chan->mbox->xfer)
		return SBI_EINVAL;

	if (!(xfer->flags & MBOX_XFER_SEQ))
		mbox_xfer_set_sequence(xfer,
			atomic_add_return(&chan->mbox->xfer_next_seq, 1));

	return chan->mbox->xfer(chan, xfer);
}
