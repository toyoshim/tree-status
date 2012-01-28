/*
 * USB CrTreeStatus driver - 0.1
 *
 * Copyright (C) 2012 TOYOSHIMA Takashi (toyoshim@gmail.com)
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License as
 *	published by the Free Software Foundation, version 2.
 *
 */

#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/usb.h>


#define DRIVER_AUTHOR	"TOYOSHIMA Takashi, toyoshim@gmail.com"
#define DRIVER_DESC	"USB CrTreeStatus Driver"

#define VENDOR_ID	0x6666
#define PRODUCT_ID	0x5110

#define TYPE_CUSTOM_OUT	0x40
#define TYPE_CUSTOM_IN	0xc0
#define REQ_PING        0
#define REQ_SET         1
#define REQ_GET         2

/* table of devices that work with this driver */
static struct usb_device_id id_table [] = {
	{ USB_DEVICE(VENDOR_ID, PRODUCT_ID) },
	{ },
};
MODULE_DEVICE_TABLE (usb, id_table);

struct usb_crts {
	struct usb_device* udev;
	unsigned int send;
	unsigned int recv;
};

static ssize_t crts_get(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct usb_interface* intf = to_usb_interface(dev);
	struct usb_crts* crts = usb_get_intfdata(intf);
	int retval;

	if (buf == NULL)
		return 0;

	// get green led status
	retval = usb_control_msg(crts->udev,
				 crts->recv,
				 REQ_GET,
				 TYPE_CUSTOM_IN,
				 0, 0,
				 &buf[0], 1,
				 USB_CTRL_GET_TIMEOUT);
	if (retval != 1)
		dev_info(&crts->udev->dev, "USB CrTreeStatus get green led status failed\n");

	// get red led status
	retval = usb_control_msg(crts->udev,
				 crts->recv,
				 REQ_GET,
				 TYPE_CUSTOM_IN,
				 0, 1,
				 &buf[1], 1,
				 USB_CTRL_GET_TIMEOUT);
	if (retval != 1)
		dev_info(&crts->udev->dev, "USB CrTreeStatus get red led status failed\n");

	return sprintf(buf, "Green: %s\nRed: %s\n",
		       (buf[0] == 0) ? "ON" : (buf[0] == 1) ? "OFF" : "FLASH",
		       (buf[1] == 0) ? "ON" : (buf[1] == 1) ? "OFF" : "FLASH");
}

static ssize_t crts_set(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct usb_interface* intf = to_usb_interface(dev);
	struct usb_crts *crts = usb_get_intfdata(intf);
	int retval;
	int index;
	int value;

	if (buf == NULL || count == 0)
		return 0;

	index = (buf[0] - '0') >> 2;
	value = (buf[0] - '0') & 0x3;

	retval = usb_control_msg(crts->udev,
				 crts->send,
				 REQ_SET,
				 TYPE_CUSTOM_OUT,
				 value, index,
				 NULL, 0,
				 USB_CTRL_SET_TIMEOUT);
	if (retval)
		dev_info(&crts->udev->dev, "USB CrTreeStatus set green led status failed\n");

	return 1;
}

static DEVICE_ATTR(crts, S_IWUGO | S_IRUGO, crts_get, crts_set);

static int crts_probe(struct usb_interface *interface, const struct usb_device_id *id)
{
	struct usb_device* udev = interface_to_usbdev(interface);
	struct usb_crts* dev = NULL;
	int retval = 0;
	unsigned char buffer = 0;

	dev = kzalloc(sizeof(struct usb_crts), GFP_KERNEL);
	if (dev == NULL) {
		dev_err(&interface->dev, "Out of memory\n");
		goto error_mem;
	}

	dev->udev = usb_get_dev(udev);
	dev->send = usb_sndctrlpipe(udev, 0);
	dev->recv = usb_rcvctrlpipe(udev, 0);

	usb_set_intfdata (interface, dev);

	retval = device_create_file(&interface->dev, &dev_attr_crts);
	if (retval)
		goto error;

	dev_info(&interface->dev, "USB CrTreeStatus device now attached\n");

	dev_info(&interface->dev, "USB CrTreeStatus ping\n");
	retval = usb_control_msg(dev->udev,
				 dev->recv,
				 REQ_PING,
				 TYPE_CUSTOM_IN,
				 86, 0,
				 &buffer, 1,
				 USB_CTRL_GET_TIMEOUT);
	if (retval < 0)
		dev_info(&interface->dev, "USB CrTreeStatus set error: %d\n", retval);
	if (buffer != 86)
		dev_info(&interface->dev, "USB CrTreeStatus invalid ping code: %d\n", buffer);
	return 0;

error:
	device_remove_file(&interface->dev, &dev_attr_crts);
	usb_set_intfdata (interface, NULL);
	usb_put_dev(dev->udev);
	kfree(dev);
error_mem:
	return retval;
}

static void crts_disconnect(struct usb_interface* interface)
{
	struct usb_crts* dev;

	dev = usb_get_intfdata (interface);

	device_remove_file(&interface->dev, &dev_attr_crts);

	/* first remove the files, then set the pointer to NULL */
	usb_set_intfdata (interface, NULL);

	usb_put_dev(dev->udev);

	kfree(dev);

	dev_info(&interface->dev, "USB CrTreeStatus device now disconnected\n");
}

static struct usb_driver crts_driver = {
	.name =		"usb_crts",
	.probe =	crts_probe,
	.disconnect =	crts_disconnect,
	.id_table =	id_table,
};

static int __init usb_crts_init(void)
{
	int retval = 0;

	retval = usb_register(&crts_driver);
	if (retval)
		err("usb_register failed. Error number %d", retval);
	return retval;
}

static void __exit usb_crts_exit(void)
{
	usb_deregister(&crts_driver);
}

module_init (usb_crts_init);
module_exit (usb_crts_exit);

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL");
