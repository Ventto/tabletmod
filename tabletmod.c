// SPDX-License-Identifier: GPL-2.0

/*
 * tabletmod.c: Tablet mode detection disabling trackpad and internal keyboard
 *
 * (C) Copyright 2019 Thomas Venriès
 * Author: Thomas Venriès <thomas@cryd.io>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; version 2
 * of the License.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/workqueue.h>
#include <linux/dmi.h>
#include <linux/limits.h>       // PATH_MAX macro
#include <linux/device.h>       // bus_find_device_by_name()
#include <linux/iio/iio.h>      // struct iio_bus_type
#include <linux/iio/consumer.h> // devm_iio_get_channel()
#include <linux/err.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Thomas Venriès <thomas@cryd.io> ");
MODULE_DESCRIPTION("Deffered task example");

#define DEFERRED_TASK_DELAY 1000

#define SCHEDULE_DELAYED_WORK(_work)                                           \
	do {                                                                   \
		if (!schedule_delayed_work(                                    \
			    _work, msecs_to_jiffies(DEFERRED_TASK_DELAY))) {   \
			pr_warn("%s(): work is already on a queue\n",          \
				__func__);                                     \
		}                                                              \
	} while (0)

// FIXME: add macro angle threshold range (min,max)

struct delayed_work accels_work;
static struct iio_dev *accel1, *accel2;

struct tabletmod_devs {
	char accels[2][PATH_MAX];
	char inputs[2][PATH_MAX];
};

static const struct tabletmod_devs system_configs[] __initconst = {
	{
		.accels = { "iio:device0", "iio:device1" },
		.inputs = { "isa0060/serio0/input0", "isa0060/serio0/input1" }
	},
};

static const struct dmi_system_id tabletmod_machines[] __initconst = {
	{
		.ident = "Ordissimo vz8",
		.matches = {
			DMI_MATCH(DMI_PRODUCT_NAME, "GeoFlex3"),
			DMI_MATCH(DMI_BOARD_NAME, "S133AR700"),
		},
		.driver_data = (void *) &system_configs[0]
	},
	{
		.ident = "Ordissimo vn1",
		.matches = {
			DMI_MATCH(DMI_PRODUCT_NAME, "Julia R37"),
			DMI_MATCH(DMI_BOARD_NAME, "Julia R37"),
		},
		.driver_data = (void *) &system_configs[0]
	},
};

static struct iio_dev *tabletmod_find_iio_dev(const char *name)
{
	struct iio_dev *indio_dev;
	struct device *dev;

	dev = bus_find_device_by_name(&iio_bus_type, NULL, name);

	if (!dev)
		return NULL;

	indio_dev = container_of(dev, struct iio_dev, dev);

	if (!indio_dev)
		return NULL;

	return indio_dev;
}

static int tabletmod_print_coordinates(struct iio_dev *indio_dev)
{
	struct iio_chan_spec const *chans;
	int i, j, val2;
	int vals[3];

	if (!indio_dev || !indio_dev->channels) {
		pr_warn("%s(): device not found\n", __func__);
		return -ENODEV;
	}

	chans = indio_dev->channels;
	for (i = 0; i < indio_dev->num_channels; ++i) {
		pr_info("%s(): %s: channel%d: type=%d\n", __func__,
			indio_dev->name, i, chans->type);
	}

	pr_info("%s(): %s: trying to read from channel0...\n", __func__,
		indio_dev->name);

	for (i = 0; i < 3; ++i) {
		chans = indio_dev->channels;
		for (j = 0; j < (indio_dev->num_channels - 1); ++j) {
			indio_dev->info->read_raw(indio_dev, chans++, &vals[j],
						  &val2, IIO_CHAN_INFO_RAW);
		}
		pr_info("%s(): %s: read_raw(channel0): (%d;%d;%d)\n", __func__,
			indio_dev->name, vals[0], vals[1], vals[2]);
		ssleep(1);
	}

	return 0;
}

static int tabletmod_find_devs(const struct dmi_system_id *dmi)
{
	struct tabletmod_devs *tab_devs = dmi->driver_data;

	accel1 =  tabletmod_find_iio_dev(tab_devs->accels[0]);

	if (!accel1) {
		pr_warn("%s(): device %s is missing\n", __func__,
			tab_devs->accels[0]);
		return -ENODEV;
	}

	accel2 =  tabletmod_find_iio_dev(tab_devs->accels[1]);

	if (!accel2) {
		pr_warn("%s(): device %s is missing\n", __func__,
			tab_devs->accels[0]);
		return -ENODEV;
	}

	// FIXME: Find input devices
	return 0;
}

static void tabletmod_work_check_angle(struct work_struct *work)
{
	// FIXME: Check angle between the two accelerometers
	// FIXME: Disable trackpad and internal keyboard

	tabletmod_print_coordinates(accel1);
	tabletmod_print_coordinates(accel2);

	SCHEDULE_DELAYED_WORK(&accels_work);
}

static int __init tabletmod_init(void)
{
	const struct dmi_system_id *dmi;
	int ret = 0;

	pr_info("%s(): begin\n", __func__);

	/* Identify the machine and verify the required devices are present */
	ret = dmi_check_system(tabletmod_machines);
	/* We expect a unique profile per machine */
	if (ret != 1) {
		pr_warn("%s(): expects a unique machine profile, but found %d.\n",
			__func__, ret);
		return 0; // Avoid kernel error
	}
	dmi = dmi_first_match(tabletmod_machines);

	if (tabletmod_find_devs(dmi) != 0) {
		pr_warn("%s(): devices are missing.\n", __func__);
		return -ENODEV;
	}

	INIT_DELAYED_WORK(&accels_work, tabletmod_work_check_angle);
	pr_info("%s(): scheduling work...\n", __func__);
	SCHEDULE_DELAYED_WORK(&accels_work);
	return 0;
}

static void __exit tabletmod_exit(void)
{
	pr_info("%s(): canceling work...\n", __func__);
	cancel_delayed_work_sync(&accels_work);
}

module_init(tabletmod_init);
module_exit(tabletmod_exit);
