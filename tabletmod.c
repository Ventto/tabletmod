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
#include <linux/dmi.h>
#include <linux/limits.h>       // define PATH_MAX
#include <linux/device.h>       // bus_find_device_by_name()
#include <linux/iio/iio.h>
#include <linux/vt_kern.h>
#include <linux/input/mousedev.h>

#define MODULE_NAME "tabletmod"
#define __debug_variable debug

#define LOG(level, format, arg...)                                             \
	do {                                                                   \
		if (__debug_variable)                                          \
			pr_##level("%s: %s(): " format "\n", MODULE_NAME,      \
				   __func__, ##arg);                           \
	} while (0)
#define INFO(format, arg...)	LOG(info, format, ##arg)
#define ERR(format, arg...)	LOG(err, format, ##arg)
#define DBG(format, arg...)	LOG(warn, format, ##arg)

#define DEFERRED_TASK_DELAY 1000

#define SCHEDULE_DELAYED_WORK(_work)                                           \
	do {                                                                   \
		if (!schedule_delayed_work(                                    \
			    _work, msecs_to_jiffies(DEFERRED_TASK_DELAY))) {   \
			pr_warn("%s(): work is already on a queue\n",          \
				__func__);                                     \
		}                                                              \
	} while (0)

static bool debug __read_mostly;

static struct delayed_work accels_work;
static bool inputs_disabled = false;

static struct accel_handler {
	struct iio_dev *dev;
	int raw_data[3];
	bool is_tablet;
} ts_hdlr, kb_hdlr;

struct tabletmod_devs {
	char accels[2][PATH_MAX];
	char inputs[2][PATH_MAX];
};
static struct tabletmod_devs system_config;
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

static int tabletmod_read_accel(struct accel_handler *accel)
{
	struct iio_chan_spec const *chans;
	int i, val2;
	struct iio_dev *indio_dev = accel->dev;

	if (!indio_dev || !indio_dev->channels) {
		DBG("device not found");
		return -ENODEV;
	}

	if (__debug_variable) {
		chans = indio_dev->channels;
		for (i = 0; i < indio_dev->num_channels; ++i)
			DBG("%s: channel%d: type=%d", indio_dev->name, i,
			    chans->type);
	}
	chans = indio_dev->channels;
	/*
	 * Let's assume that the number of channels is based on the following
	 * scheme: num_channels = N * IIO_ACCEL + IIO_TIMESTAMP
	 * where:
	 *	- `IIO_ACCEL`: an axis data
	 *	- `IIO_TIMESTAMP`: a delay data.
	 * and `IIO_ACCEL` channels are leading the channel list
	 *
	 * So, the first `num_channels - 1` element(s) of
	 * the `struct iio_chan_spec` list are the axis data we need to read.
	 */
	for (i = 0; i < (indio_dev->num_channels - 1); ++i) {
		indio_dev->info->read_raw(indio_dev, chans++,
					  &accel->raw_data[i], &val2,
					  IIO_CHAN_INFO_RAW);
	}
	DBG("%s: (%d;%d;%d)", indio_dev->name, accel->raw_data[0],
	    accel->raw_data[1], accel->raw_data[2]);

	return 0;
}

static struct iio_dev *tabletmod_find_iio_by_name(const char *name)
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

static int tabletmod_check_devices(const struct dmi_system_id *dmi)
{
	struct tabletmod_devs *tab_devs = dmi->driver_data;

	ts_hdlr.dev =  tabletmod_find_iio_by_name(tab_devs->accels[0]);

	if (!ts_hdlr.dev) {
		DBG("device %s is missing", tab_devs->accels[0]);
		return -ENODEV;
	}

	kb_hdlr.dev =  tabletmod_find_iio_by_name(tab_devs->accels[1]);

	if (!kb_hdlr.dev) {
		DBG("device %s is missing", tab_devs->accels[1]);
		return -ENODEV;
	}
	return 0;
}

static void tabletmod_disable_inputs(bool disabled)
{
	if (disabled == inputs_disabled)
		return;

	INFO("%s inputs", (disabled) ? "disabled" : "enabled");

	kd_disable(disabled, system_config.inputs[0]);
	mousedev_disable(disabled, system_config.inputs[1]);
	inputs_disabled = disabled;
}

static inline bool tabletmod_touchscreen_istablet(struct accel_handler *accel)
{
	/* XZ Rotation: forward */
	return (accel->raw_data[1] < 0 && accel->raw_data[2] < 500)
		/* XY Rotation: left or right */
		|| (accel->raw_data[1] < 320 && (accel->raw_data[0] > 380
			|| accel->raw_data[0] < -380))
		/* XYZ Rotation: forward-left */
		|| (accel->raw_data[0] > -150 && accel->raw_data[1] < 250
			&& accel->raw_data[2] > 360);
}

static inline bool tabletmod_keyboard_istablet(struct accel_handler *accel)
{	/* XZ Rotation: forward */
	return (accel->raw_data[0] < 0 && accel->raw_data[2] > -400)
		/* XZ Rotation: backward */
		|| (accel->raw_data[0] < 410 && accel->raw_data[2] > 280)
		/* YZ Rotation: right */
		|| (accel->raw_data[1] > -445 && accel->raw_data[2] > -260)
		/* YZ Rotation: left */
		|| (accel->raw_data[1] > 480 && accel->raw_data[2] > -230)
		/* XYZ Rotation: forward-left */
		|| (accel->raw_data[0] > 220 && accel->raw_data[1] < -350
			&& accel->raw_data[2] > -335);
}

static void tabletmod_handler(struct work_struct *work)
{
	if (tabletmod_read_accel(&ts_hdlr) != 0
			|| tabletmod_read_accel(&kb_hdlr) != 0) {
		ERR("cannot read from one or both accelerometers");
		goto schedule;
	}

	tabletmod_disable_inputs(tabletmod_touchscreen_istablet(&ts_hdlr) ||
			         tabletmod_keyboard_istablet(&kb_hdlr));

schedule:
	SCHEDULE_DELAYED_WORK(&accels_work);
}

static int __init tabletmod_init(void)
{
	const struct dmi_system_id *dmi;
	int ret;

	/* Identify the machine and verify the required devices are present */
	ret = dmi_check_system(tabletmod_machines);
	/* We expect a unique profile per machine */
	if (ret != 1) {
		ERR("expects a unique machine profile, but found %d.", ret);
		return 0; // Avoid kernel error
	}
	dmi = dmi_first_match(tabletmod_machines);

	if (tabletmod_check_devices(dmi) != 0) {
		ERR("some devices are missing");
		return -ENODEV;
	}
	/* Store the system config after checking */
	system_config = *(struct tabletmod_devs *)dmi->driver_data;

	INIT_DELAYED_WORK(&accels_work, tabletmod_handler);
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

MODULE_AUTHOR("Thomas Venriès <thomas@cryd.io>");
MODULE_VERSION("0.1");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Detect the tablet mode from accelerometers and disable inputs accordingly");

module_param(debug, bool, 0644);
MODULE_PARM_DESC(debug, "Enable debug messages");
