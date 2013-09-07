/*
 * ChromeOS EC multi-function device
 *
 * Copyright (C) 2012 Google, Inc
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * The ChromeOS EC multi function device is used to mux all the requests
 * to the EC device for its multiple features: keyboard controller,
 * battery charging and regulator control, firmware update.
 */

#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/mfd/core.h>
#include <linux/mfd/cros_ec.h>
#include <linux/mfd/cros_ec_commands.h>
#include <linux/module.h>

int cros_ec_prepare_tx(struct cros_ec_device *ec_dev,
		       struct cros_ec_command *msg)
{
	uint8_t *out;
	int csum, i;

	BUG_ON(msg->outsize > EC_PROTO2_MAX_PARAM_SIZE);
	out = ec_dev->dout;
	out[0] = EC_CMD_VERSION0 + msg->version;
	out[1] = msg->command;
	out[2] = msg->outsize;
	csum = out[0] + out[1] + out[2];
	for (i = 0; i < msg->outsize; i++)
		csum += out[EC_MSG_TX_HEADER_BYTES + i] = msg->outdata[i];
	out[EC_MSG_TX_HEADER_BYTES + msg->outsize] = (uint8_t)(csum & 0xff);

	return EC_MSG_TX_PROTO_BYTES + msg->outsize;
}

static struct mfd_cell cros_devs[] = {
	{
		.name = "cros-ec-keyb",
		.id = 1,
		.of_compatible = "google,cros-ec-keyb",
	},
	{
		.name = "cros-ec-i2c-tunnel",
		.id = 2,
		.of_compatible = "google,cros-ec-i2c-tunnel",
	},
	{
		.name = "cros-ec-dev",
		.id = 3,
	},
	{
		.name = "cros-ec-vbc",
		.id = 4,
		.of_compatible = "google,cros-ec-vbc",
	},
	{
		.name = "cros-ec-tps65090",
		.id = 5,
		.of_compatible = "ti,cros-ec-tps65090",
	},
};

int cros_ec_register(struct cros_ec_device *ec_dev)
{
	struct device *dev = ec_dev->dev;
	int err = 0;

	if (ec_dev->din_size) {
		ec_dev->din = kmalloc(ec_dev->din_size, GFP_KERNEL);
		if (!ec_dev->din) {
			err = -ENOMEM;
			goto fail_din;
		}
	}
	if (ec_dev->dout_size) {
		ec_dev->dout = kmalloc(ec_dev->dout_size, GFP_KERNEL);
		if (!ec_dev->dout) {
			err = -ENOMEM;
			goto fail_dout;
		}
	}

	err = mfd_add_devices(dev, 0, cros_devs,
			      ARRAY_SIZE(cros_devs),
			      NULL, ec_dev->irq, NULL);
	if (err) {
		dev_err(dev, "failed to add mfd devices\n");
		goto fail_mfd;
	}

	dev_info(dev, "Chrome EC device registered\n");

	return 0;

fail_mfd:
	if (ec_dev->dout_size)
		kfree(ec_dev->dout);
fail_dout:
	if (ec_dev->din_size)
		kfree(ec_dev->din);
fail_din:
	return err;
}
EXPORT_SYMBOL(cros_ec_register);

int cros_ec_remove(struct cros_ec_device *ec_dev)
{
	mfd_remove_devices(ec_dev->dev);
	if (ec_dev->dout_size)
		kfree(ec_dev->dout);
	if (ec_dev->din_size)
		kfree(ec_dev->din);

	return 0;
}
EXPORT_SYMBOL(cros_ec_remove);

#ifdef CONFIG_PM_SLEEP
int cros_ec_suspend(struct cros_ec_device *ec_dev)
{
	struct device *dev = ec_dev->dev;

	if (device_may_wakeup(dev))
		ec_dev->wake_enabled = !enable_irq_wake(ec_dev->irq);

	disable_irq(ec_dev->irq);
	ec_dev->was_wake_device = ec_dev->wake_enabled;

	return 0;
}

int cros_ec_resume(struct cros_ec_device *ec_dev)
{
	enable_irq(ec_dev->irq);

	if (ec_dev->wake_enabled) {
		disable_irq_wake(ec_dev->irq);
		ec_dev->wake_enabled = 0;
	}

	return 0;
}
#endif
