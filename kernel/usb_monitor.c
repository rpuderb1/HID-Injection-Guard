// USB HID Device Monitor Kernel Module

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/usb.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>

// Module metadata
MODULE_LICENSE("GPL");
MODULE_AUTHOR("USB HID Security Project");
MODULE_DESCRIPTION("USB HID Device Monitor");
MODULE_VERSION("1.0");

// Store last HID device information
static u16 last_vid = 0;
static u16 last_pid = 0;
static char last_manufacturer[128] = "none";
static char last_product[128] = "none";
static char last_serial[128] = "none";

// For sysfs directory
static struct kobject *usb_hid_kobj;

// Sysfs attribute show functions
static ssize_t vid_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) {
    return sprintf(buf, "0x%04x\n", last_vid);
}

static ssize_t pid_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) {
    return sprintf(buf, "0x%04x\n", last_pid);
}

static ssize_t manufacturer_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) {
    return sprintf(buf, "%s\n", last_manufacturer);
}

static ssize_t product_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) {
    return sprintf(buf, "%s\n", last_product);
}

static ssize_t serial_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) {
    return sprintf(buf, "%s\n", last_serial);
}

// sysfs attributes
static struct kobj_attribute vid_attribute = __ATTR_RO(vid);
static struct kobj_attribute pid_attribute = __ATTR_RO(pid);
static struct kobj_attribute manufacturer_attribute = __ATTR_RO(manufacturer);
static struct kobj_attribute product_attribute = __ATTR_RO(product);
static struct kobj_attribute serial_attribute = __ATTR_RO(serial);

// Attribute group
static struct attribute *attrs[] = {
    &vid_attribute.attr,
    &pid_attribute.attr,
    &manufacturer_attribute.attr,
    &product_attribute.attr,
    &serial_attribute.attr,
    NULL,
};

static struct attribute_group attr_group = {
    .attrs = attrs,
};

// Check if USB device has HID interface
static bool is_hid_device(struct usb_device *udev) {
    int config_idx, interface_idx;
    struct usb_host_config *config;
    struct usb_interface *interface;
    struct usb_interface_descriptor *desc;

    // Iterate through all configurations
    for (config_idx = 0; config_idx < udev->descriptor.bNumConfigurations; config_idx++) {
        config = &udev->config[config_idx];

        // Iterate through all interfaces in this configuration
        for (interface_idx = 0; interface_idx < config->desc.bNumInterfaces; interface_idx++) {
            interface = config->interface[interface_idx];
            if (!interface)
                continue;

            // Get interface descriptor
            desc = &interface->altsetting[0].desc;

            // Check if interface is HID class (0x03)
            if (desc->bInterfaceClass == USB_CLASS_HID) {
                return true;
            }
        }
    }

    return false;
}

// Called whenever a USB device is added or removed
static int usb_notify_callback(struct notifier_block *self, unsigned long action, void *dev) {
    struct usb_device *udev = dev;

    // Only process HID devices
    if (!is_hid_device(udev)) {
        return NOTIFY_OK;
    }

    switch (action) {
    case USB_DEVICE_ADD:
        printk(KERN_INFO "usb_monitor: HID device connected\n");

        // Extract & store device information
        last_vid = le16_to_cpu(udev->descriptor.idVendor);
        last_pid = le16_to_cpu(udev->descriptor.idProduct);

        printk(KERN_INFO "usb_monitor:   VID: 0x%04x\n", last_vid);
        printk(KERN_INFO "usb_monitor:   PID: 0x%04x\n", last_pid);

        // Store manufacturer string if available
        if (udev->manufacturer) {
            snprintf(last_manufacturer, sizeof(last_manufacturer), "%s", udev->manufacturer);
            printk(KERN_INFO "usb_monitor:   Manufacturer: %s\n", last_manufacturer);
        } else {
            snprintf(last_manufacturer, sizeof(last_manufacturer), "none");
        }

        // Store product string if available
        if (udev->product) {
            snprintf(last_product, sizeof(last_product), "%s", udev->product);
            printk(KERN_INFO "usb_monitor:   Product: %s\n", last_product);
        } else {
            snprintf(last_product, sizeof(last_product), "none");
        }

        // Store serial number if available
        if (udev->serial) {
            snprintf(last_serial, sizeof(last_serial), "%s", udev->serial);
            printk(KERN_INFO "usb_monitor:   Serial: %s\n", last_serial);
        } else {
            snprintf(last_serial, sizeof(last_serial), "none");
        }

        break;

    case USB_DEVICE_REMOVE:
        printk(KERN_INFO "usb_monitor: HID device disconnected\n");
        printk(KERN_INFO "usb_monitor:   VID: 0x%04x, PID: 0x%04x\n",
               le16_to_cpu(udev->descriptor.idVendor),
               le16_to_cpu(udev->descriptor.idProduct));
        break;
    }
    return NOTIFY_OK;
}

// Links callback to the USB subsystem
static struct notifier_block usb_notify = {
    .notifier_call = usb_notify_callback,
};

// This function will be called when the module is loaded (insmod)
static int __init usb_monitor_init(void) {
    int retval;

    printk(KERN_INFO "usb_monitor: Module loaded!\n");

    // Create kobject for sysfs directory /sys/kernel/usb_hid_monitor
    usb_hid_kobj = kobject_create_and_add("usb_hid_monitor", kernel_kobj);
    if (!usb_hid_kobj) {
        printk(KERN_ERR "usb_monitor: Failed to create kobject\n");
        return -ENOMEM;
    }

    // Create sysfs attribute files
    retval = sysfs_create_group(usb_hid_kobj, &attr_group);
    if (retval) {
        printk(KERN_ERR "usb_monitor: Failed to create sysfs group\n");
        kobject_put(usb_hid_kobj);
        return retval;
    }

    printk(KERN_INFO "usb_monitor: Sysfs entries created at /sys/kernel/usb_hid_monitor/\n");

    // Register notifier to receive USB events
    usb_register_notify(&usb_notify);
    printk(KERN_INFO "usb_monitor: USB notifier registered\n");

    return 0;
}

// This function will be called when the module is unloaded (rmmod)
static void __exit usb_monitor_exit(void) {
    // Unregister USB notifier
    usb_unregister_notify(&usb_notify);
    printk(KERN_INFO "usb_monitor: USB notifier unregistered\n");

    // Remove sysfs entries
    sysfs_remove_group(usb_hid_kobj, &attr_group);
    kobject_put(usb_hid_kobj);
    printk(KERN_INFO "usb_monitor: Sysfs entries removed\n");

    printk(KERN_INFO "usb_monitor: Module unloaded!\n");
}

// Register init and exit functions
module_init(usb_monitor_init);
module_exit(usb_monitor_exit);