// SPDX-License-Identifier: GPL-2.0

/*
 * tabletmod.c: Detects that a 2-in-1 hybrid laptop is in tablet mode
 *              and disables the internal keyboard and trackpad accordingly.
 *
 * (C) Copyright 2019 Thomas Venriès
 * Author: Thomas Venriès <thomas@cryd.io>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; version 2
 * of the License.
 */

#include <linux/kernel.h>         // abs()
#include <linux/module.h>
#include <linux/dmi.h>
#include <linux/iio/iio.h>
#include <linux/device.h>         // bus_find_device_by_name()
#include <linux/vt_kern.h>        // kd_disable()
#include <linux/input/mousedev.h> // mousedev_disable()
#include <linux/limits.h>         // PATH_MAX macro
#include <acpi/button.h>          // acpi_lid_open()

#define __debug_variable debug

#define LOG(level, format, arg...)                                             \
	do {                                                                   \
		if (__debug_variable)                                          \
			pr_##level("Tabletmod: %s(): " format "\n", __func__,  \
				   ##arg);                                     \
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
	struct	iio_dev *dev;
	int	raw_data[3];
	bool	is_tablet;
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
		.ident = "Ordissimo Julia 2",
		.matches = {
			DMI_MATCH(DMI_PRODUCT_NAME, "GeoFlex3"),
			DMI_MATCH(DMI_BOARD_NAME, "S133AR700"),
		},
		.driver_data = (void *) &system_configs[0]
	},
};

/*
 * Reads the raw values (Ax, Ay and Az) from a given accelerometer.
 */
static int tabletmod_read_accel(struct accel_handler *accel)
{
	struct iio_chan_spec const *chans;
	int i, val2;
	struct iio_dev *indio_dev = accel->dev;

	if (!indio_dev || !indio_dev->channels) {
		DBG("device not found");
		return -ENODEV;
	}

	/* FIXME: Selects only the `IIO_ACCEL` channels and calls read_raw()
	 * function.
	 */
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

/*
 * Finds a industrial I/O device (or accelerometer in our case) by its name.
 */
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

	/* TODO: A IIO device must have at least 2 or 3 x IIO_ACCEL-type
	 *       channels. Otherwise, we return NULL.
	 */
	return indio_dev;
}

/*
 * Verifies if the devices specified in the system config exist.
 */
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

/*
 * Disables both the 2-in-1 laptop's keyboard and trackpad.
 */
static void tabletmod_disable_inputs(bool disabled)
{
	if (disabled == inputs_disabled)
		return;

	INFO("%s inputs", (disabled) ? "disabled" : "enabled");

	/*
	 * TODO: We should access the internal keyboard and trackpad
	 * input devices without the need of patching the kernel code base:
	 */
	kd_disable(disabled, system_config.inputs[0]);
	mousedev_disable(disabled, system_config.inputs[1]);
	inputs_disabled = disabled;
}

/*
 * Detects that the 2-in-1 laptop's touchscreen is in tablet position.
 */
static inline bool detect_tabletmode_touchscreen(struct accel_handler *accel)
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

/*
 * Detects that the laptop's keyboard is in tablet position.
 */
static inline bool detect_tabletmode_keyboard(struct accel_handler *accel)
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

static u16 distance(int a, int b)
{
	if (a > 0 && b > 0)
		return max(a, b) - min(a, b);
	else if (a < 0 && b < 0)
		return abs(a - b);
	else if (a == b)
		return 0;
	return abs(a) + abs(b);
}

static inline bool detect_tabletmode_parallel(struct accel_handler *accel1,
					      struct accel_handler *accel2)
{
	/*
	 * When the touchscreen is folded back at 360° and facing the ground
	 * it is the same position than a close laptop put on the desk.
	 * That's why if the lid is open, we consider a range of position
	 * as tablet mode.
	 */
	return acpi_lid_open() &&
	       distance(accel1->raw_data[2], accel2->raw_data[2]) < 100;
}

/*
 * Delayed task function which disables the input devices if it detects that
 * the laptop is in tablet mode.
 */
static void tabletmod_handler(struct work_struct *work)
{
	if (tabletmod_read_accel(&ts_hdlr) != 0
			|| tabletmod_read_accel(&kb_hdlr) != 0) {
		ERR("cannot read from one or both accelerometers");
		goto schedule;
	}

	tabletmod_disable_inputs(
		detect_tabletmode_parallel(&ts_hdlr, &kb_hdlr) ||
		detect_tabletmode_touchscreen(&ts_hdlr) ||
		detect_tabletmode_keyboard(&kb_hdlr));

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
