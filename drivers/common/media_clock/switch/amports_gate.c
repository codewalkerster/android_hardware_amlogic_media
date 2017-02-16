/*
 * drivers/amlogic/media/common/arch/switch/amports_gate.c
 *
 * Copyright (C) 2016 Amlogic, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
*/
#include <linux/compiler.h>
#include <linux/clk-provider.h>
#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/clk.h>
#include "amports_gate.h"
#include <linux/amlogic/media/utils/vdec_reg.h>
#include "../../../stream_input/amports/amports_priv.h"

#define DEBUG_REF 0
#define GATE_RESET_OK

struct gate_swtch_node {
	struct clk *clk;
	const char *name;
	spinlock_t lock;
	unsigned long flags;
	int ref_count;
};
#ifdef GATE_RESET_OK

struct gate_swtch_node gates[] = {
	{
		.name = "demux",
	},
	{
		.name = "parser_top",
	},
	{
		.name = "vpu_intr",
	},
	{
		.name = "vdec",
	},
	{
		.name = "clk_vdec_mux",
	},
	{
		.name = "clk_hcodec_mux",
	},
	{
		.name = "clk_hevc_mux",
	},
};


/*
mesonstream {
	compatible = "amlogic, codec, streambuf";
	dev_name = "mesonstream";
	status = "okay";
	clocks = <&clkc CLKID_DOS_PARSER
		&clkc CLKID_VPU_INTR
		&clkc CLKID_DEMUX
		&clkc CLKID_DOS
		&clkc CLKID_VDEC_MUX
		&clkc CLKID_HCODEC_MUX
		&clkc CLKID_HEVC_MUX>;
	clock-names = "parser_top",
		"vpu_intr",
		"demux",
		"vdec",
		"clk_vdec_mux",
		"clk_hcodec_mux",
		"clk_hevc_mux";
};
*/

int amports_clock_gate_init(struct device *dev)
{
	int i;

	for (i = 0; i < sizeof(gates) / sizeof(struct gate_swtch_node); i++) {
		gates[i].clk = devm_clk_get(dev, gates[i].name);
		if (IS_ERR_OR_NULL(gates[i].clk)) {
			gates[i].clk = NULL;
			pr_info("get gate %s control failed %p\n",
				gates[i].name,
				gates[i].clk);
		} else {
			pr_info("get gate %s control ok %p\n",
				gates[i].name,
				gates[i].clk);
		}
		gates[i].ref_count = 0;
		spin_lock_init(&gates[i].lock);
	}
	return 0;
}
EXPORT_SYMBOL(amports_clock_gate_init);

static int amports_gate_clk(struct gate_swtch_node *gate_node, int enable)
{
	spin_lock_irqsave(&gate_node->lock, gate_node->flags);
	if (enable) {
		if (DEBUG_REF)
			pr_info("amports_gate_reset,count: %d\n",
				gate_node->ref_count);
		if (gate_node->ref_count == 0)
			clk_prepare_enable(gate_node->clk);

		gate_node->ref_count++;
	} else {
		gate_node->ref_count--;
		if (DEBUG_REF)
			pr_info("amports_gate_reset,count: %d\n",
				gate_node->ref_count);

		if (gate_node->ref_count == 0)
			clk_disable_unprepare(gate_node->clk);
	}
	spin_unlock_irqrestore(&gate_node->lock, gate_node->flags);
	return 0;
}

int amports_switch_gate(const char *name, int enable)
{
	int i;

	for (i = 0; i < sizeof(gates) / sizeof(struct gate_swtch_node); i++) {
		if (!strcmp(name, gates[i].name)) {

			/*pr_info("openclose:%d gate %s control\n", enable,
				gates[i].name);*/

			if (gates[i].clk)
				amports_gate_clk(&gates[i], enable);
		}
	}
	return 0;
}
EXPORT_SYMBOL(amports_switch_gate);

#else
/*
*can used for debug.
*on chip bringup.
*/
int amports_clock_gate_init(struct device *dev)
{
	static int gate_inited;

	if (gate_inited)
		return 0;
/*
*#define HHI_GCLK_MPEG0    0x1050
*#define HHI_GCLK_MPEG1    0x1051
*#define HHI_GCLK_MPEG2    0x1052
*#define HHI_GCLK_OTHER    0x1054
*#define HHI_GCLK_AO       0x1055
*/
	WRITE_HHI_REG_BITS(HHI_GCLK_MPEG0, 1, 1, 1);/*dos*/
	WRITE_HHI_REG_BITS(HHI_GCLK_MPEG1, 1, 25, 1);/*U_parser_top()*/
	WRITE_HHI_REG_BITS(HHI_GCLK_MPEG1, 0xff, 6, 8);/*aiu()*/
	WRITE_HHI_REG_BITS(HHI_GCLK_MPEG1, 1, 4, 1);/*demux()*/
	WRITE_HHI_REG_BITS(HHI_GCLK_MPEG1, 1, 2, 1);/*audio in()*/
	WRITE_HHI_REG_BITS(HHI_GCLK_MPEG2, 1, 25, 1);/*VPU Interrupt*/
	gate_inited++;



	return 0;
}
EXPORT_SYMBOL(amports_clock_gate_init);

static int amports_switch_gate(struct gate_swtch_node *gate_node, int enable)
{
	return 0;
}

int amports_switch_gate(const char *name, int enable)
{
	amports_switch_gate(0, 0);
	return 0;
}
EXPORT_SYMBOL(amports_switch_gate);

#endif