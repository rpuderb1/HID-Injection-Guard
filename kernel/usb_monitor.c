// USB HID Device Monitor Kernel Module

#include <linux/module.h>
#include <linux/kernel.h>

// Module metadata
MODULE_LICENSE("GPL");
MODULE_AUTHOR("USB HID Security Project");
MODULE_DESCRIPTION("USB HID Device Monitor");
MODULE_VERSION("1.0");


// This function will be called when the module is loaded (insmod)
static int __init usb_monitor_init(void) {
    printk(KERN_INFO "usb_monitor: Module loaded!\n");
    return 0;
}


// This function will be called when the module is unloaded (rmmod)
static void __exit usb_monitor_exit(void) {
    printk(KERN_INFO "usb_monitor: Module unloaded!\n");
}

// Register init and exit functions
module_init(usb_monitor_init);
module_exit(usb_monitor_exit);