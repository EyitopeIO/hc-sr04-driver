/*
* Eyitope Adelowo (adelowoe@coventry.ac.uk)
* 19-FEB-2022
*
* -------------
* DESCRIPTION
* -------------
* Driver for HC-SR04 ultrasonic sensor usigng sys file system (SYS) and virtual file system (VFS)
*
*/


#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/types.h>.
#include <linux/kdev_t.h>
#include <linux/fs.h>

static dev_t majorN;
static cdev minorN;

// MAJOR(dev_t dev);
// MINOR(dev_t dev);
// MKDEV(major_num, minor_num);

struct file_operations hcsr04_ops = {
    .owner = THIS_MODULE;
    .read = hcsr04_read;
    .write = hcsr04_write;
}


static int __init hcsro4_init(void) 
{
    alloc_chrdev_region(&majorN, 0, 1, "hcsr04");
    cdev_init(&minorN, &hcsr04_ops);
    hello_cdev.owner = THIS_MODULE;
    cdev_add(&minorN, )
    return 0;
}


