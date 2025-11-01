// USB HID Device Monitor Kernel Module

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/usb.h>

// Module metadata
MODULE_LICENSE("GPL");
MODULE_AUTHOR("USB HID Security Project");
MODULE_DESCRIPTION("USB HID Device Monitor");
MODULE_VERSION("1.0");


// Called whenever a USB device is added or removed
static int usb_notify_callback(struct notifier_block *self, unsigned long action, void *dev) {
    struct usb_device *udev = dev;

    switch (action) {
    case USB_DEVICE_ADD:
        printk(KERN_INFO "usb_monitor: USB device connected\n");

        // Extract device information from device descriptor
        printk(KERN_INFO "usb_monitor:   VID: 0x%04x\n",
               le16_to_cpu(udev->descriptor.idVendor));
        printk(KERN_INFO "usb_monitor:   PID: 0x%04x\n",
               le16_to_cpu(udev->descriptor.idProduct));

        // Log manufacturer string if available
        if (udev->manufacturer) {
            printk(KERN_INFO "usb_monitor:   Manufacturer: %s\n",
                   udev->manufacturer);
        }

        // Log product string if available
        if (udev->product) {
            printk(KERN_INFO "usb_monitor:   Product: %s\n",
                   udev->product);
        }

        // Log serial number if available
        if (udev->serial) {
            printk(KERN_INFO "usb_monitor:   Serial: %s\n",
                   udev->serial);
        }

        break;

    case USB_DEVICE_REMOVE:
        printk(KERN_INFO "usb_monitor: USB device disconnected\n");
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
    printk(KERN_INFO "usb_monitor: Module loaded!\n");

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

    printk(KERN_INFO "usb_monitor: Module unloaded!\n");
}

// Register init and exit functions
module_init(usb_monitor_init);
module_exit(usb_monitor_exit);