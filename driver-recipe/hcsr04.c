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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>

#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/ktime.h>
#include <linux/device.h>
#include <linux/time64.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <asm/uaccess.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/string.h>

#include "hcsr04.h"

#define TRIG_pin 23             // hcsr04 trigger pin
#define ECHO_pin 24             // hcsr04 echo pin
#define TRIG_tme 10             // hcsr04 trigger period
#define ECHO_tmo 1000000000

static dev_t hcsr04_devt;
struct cdev hcsr04_cdev; 
static char buffer[64];
unsigned long not_copied = 0;
int pending_read = 0;
char *hcsr04_name = "hcsr04";
struct class *hcsr04_class;
struct device *hcsr04_device;
size_t le;
ktime_t ECHO_high_starttime;
ktime_t ECHO_high_stooptime;
ktime_t tmp_ktime;
ktime_t TRIG_timeout;

// struct user_data {
//     struct hcsr04_data data;
//     struct list_head giant_baby_head;
// };

// struct user_data *gbp = NULL;
struct hcsr04_data usd;

// LIST_HEAD(lifo_head);

/*
* Take full control of GPIO 23 & 24 by configuring for input (echo) and output (trigger),
* and define interrupt for input
*/
int hcsr04_open(struct inode *inode, struct file *file) 
{   
    pending_read = 0;
}

int hcsr04_release(struct inode *inode, struct file *file) 
{
    return 0;
}

ssize_t hcsr04_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos) 
{
    // struct hcsr04_data tmpbuff[MAX_READS];
    // struct hcsr04_data emptyd = {   // Represents a NULL or unavailable data.
    //     .t_stamp = 0,
    //     .t_high = 0,
    // };   
    // struct list_head *i, *tmp;
    // struct user_data *usd;
    // size_t counter = count / sizeof(struct hcsr04_data);
    
    // if (count == 0) return 0;
    
    // list_for_each_safe(i, tmp, &lifo_head) 
    // { 
    //     usd = list_entry(i, struct user_data, giant_baby_head);
    //     if (usd == NULL) tmpbuff[counter++] = emptyd;
    //     else tmpbuff[counter++] = usd->data;      
    //     list_del(i);
    //     kfree(usd);    
    //     if ((counter * sizeof(struct hcsr04_data)) == count) break;
    // }  
    // not_copied = copy_to_user(buf, tmpbuff, sizeof(tmpbuff)*counter);
    
    not_copied = copy_to_user(buf, &usd, sizeof(usd));
    pending_read = 0;
    return sizeof(usd);
}

ssize_t hcsr04_write(struct file *filp, const  char *buffer, size_t length, loff_t * offset)
{

    if (pending_read)
        return -1;
    
    gpio_set_value(TRIG_pin, 1);
    udelay(TRIG_tme);   // Wait 10 micro seconds
    gpio_set_value(TRIG_pin, 0);

    pending_read = 1;

    tmp_ktime = ktime_get_real_ns();

    while(ktime_sub(ktime_get_real_ns(), tmp_ktime) < TRIG_timeout) {

        if (gpio_get_value(ECHO_pin)) {

            ECHO_high_starttime = ktime_get_real_ns();
            
            while (ktime_sub(ktime_get_real_ns(), ECHO_high_starttime) < TRIG_timeout) {
               
                if (!gpio_get_value(ECHO_pin)) {
                    
                    ECHO_high_stooptime = ktime_get_real_ns();
                    tmp_ktime = ktime_sub(ECHO_high_stooptime, ECHO_high_starttime);
                    usd.t_stamp = (unsigned long)ktime_get_seconds();      
                    usd.t_high = (unsigned int)ktime_to_ns(tmp_ktime);
                    return 0;
                    // if ((gbp = kmalloc(sizeof(struct user_data), GFP_KERNEL)) != NULL) {
                    //     (gbp->data).t_stamp = (unsigned long)ktime_get_seconds();      
                    //     tmp_ktime = ktime_sub(ECHO_high_stooptime, ECHO_high_starttime);
                    //     (gbp->data).t_high = (unsigned int)ktime_to_ns(tmp_ktime);
                    //     list_add(&gbp->giant_baby_head, &lifo_head);
                    //     return 0;
                    // }
                }
            }
        }
    }
    return -1;
}

struct file_operations hcsr04_fops = {
    .owner = THIS_MODULE,
    .open = hcsr04_open,
    .read = hcsr04_read,
    .write = hcsr04_write,
    .release = hcsr04_release
};

static int __init hcsr04_init(void)  
{
    int errn = 0;
    int error_n = 0;

    /* Initialise device */
    if ((errn = alloc_chrdev_region(&hcsr04_devt, 0, 1, "hcsr04")) < 0) return errn;
    cdev_init(&hcsr04_cdev, &hcsr04_fops);
    hcsr04_cdev.owner = THIS_MODULE;
    
    errn = cdev_add(&hcsr04_cdev, hcsr04_devt, 1);
    if (errn) {
        printk(KERN_WARNING "Failed to add module\n");
        return errn;
    }

    hcsr04_class = class_create(THIS_MODULE, hcsr04_name);
    if (IS_ERR(hcsr04_class)) {
        printk(KERN_WARNING "Failed to create class\n");
        errn = PTR_ERR(hcsr04_class);
        return errn;
    }

    /* Add entry into node in /root/dev using class created above */
    hcsr04_device = device_create(hcsr04_class, NULL, hcsr04_devt, NULL, hcsr04_name);
    if (IS_ERR(hcsr04_device)) {
        errn = PTR_ERR(hcsr04_device);
        printk(KERN_WARNING "Failed to create device\n");
        return errn;
    }

    if ((error_n = gpio_request(TRIG_pin, "Trigger pin")) < 0 ) return error_n;
    if ((error_n = gpio_request(ECHO_pin, "Echo pin")) < 0 ) return error_n;
    if ((error_n = gpio_direction_input(ECHO_pin)) < 0 ) return error_n;
    if ((error_n = gpio_direction_output(TRIG_pin, 0)) < 0 ) return error_n;
    
    TRIG_timeout = ktime_set(0, ECHO_tmo); // Set timeout to 1 second
    
    printk(KERN_INFO "%s loaded.\n", format_dev_t(buffer, hcsr04_devt));
    return 0;
}

static void __exit hcsr04_exit(void)
{
    gpio_free(ECHO_pin);
    gpio_free(TRIG_pin);
    device_destroy(hcsr04_class, hcsr04_devt);
    cdev_del(&hcsr04_cdev);
    unregister_chrdev_region(hcsr04_devt, 1);
}


module_init(hcsr04_init);
module_exit(hcsr04_exit);
MODULE_AUTHOR("Eyitope Adelowo");
MODULE_LICENSE("GPL");
