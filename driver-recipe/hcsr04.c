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
#include <linux/time.h>
#include <linux/cdev.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/ktime.h>
#include <linux/kfifo.h>
#include <linux/list.h>
#include <linux/device.h>
#include <linux/time64.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <asm/uaccess.h>
#include <linux/gpio.h>
#include <linux/string.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>

#include "hcsr04.h"

#define TRIG_pin 23             // To hcsr04 trigger pin
#define ECHO_pin 24             // hcsr04 echo pin
#define TRIG_tme 10             // hcsr04 trigger period
#define ECHO_tmo 1000000000     // hcsr04 max echo delay allowed
#define CNST_div 58             // Division constant
#define USER_mrq 5              // User max request

/*-------- Declaration & Defs -----------*/

ssize_t hcsr04_sysfs_show(struct device *dev, struct device_attribute *attr, char *buf);
ssize_t hcsr04_sysfs_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);

static dev_t hcsr04_devt;
struct cdev hcsr04_cdev; 
static unsigned long not_copied = 0;
static char buffer[64];
static int pending_read = 0;
char *hcsr04_name = "hcsr04";
char *hcsr04_rootname = "rangesensor";
struct class *hcsr04_class;
struct device *hcsr04_device;
struct device *hcsr04_device_root;
struct timespec present_timestamp;

ktime_t ECHO_high_starttime;
ktime_t ECHO_high_stooptime;
ktime_t tmp_ktime;
ktime_t TRIG_timeout;
struct hcsr04_data usd;                     // user space data

/* The lifo */
struct hcsr04_lifo_node
{
    struct hcsr04_sysfs_ldata data;
    struct list_head mylist;
};

struct hcsr04_lifo_node *lnode, *tmp_node;

struct hcsr04_sysfs_ldata bucket[USER_mrq];

struct hcsr04_sysfs_ldata bucket_data = {
    .t_stamp = 0,
    .t_high = 0,
    .m_dist = 0
};

int number_of_nodes = 0;
int list_busy = 0;

LIST_HEAD(head_node);

struct device_attribute last5 = {
    .attr = {
        .name = "last5readings",
        .mode = 0666
    },
    .show = hcsr04_sysfs_show,
    .store = hcsr04_sysfs_store
};


/* ------------------------------------------*/


/*
* sysfs definitions below
*/

ssize_t hcsr04_sysfs_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    ssize_t ret = 0;
    int i;

    struct hcsr04_sysfs_ldata btmp;
    struct hcsr04_lifo_node *cursor, *tmp;

    while (list_busy);

    list_busy = 1;
    i = 0;

    list_for_each_entry_safe(cursor, tmp, &head_node, mylist) {

        if (cursor == NULL) break;

        btmp.t_stamp = (cursor->data).t_stamp;
        btmp.t_high = (cursor->data).t_high;
        btmp.m_dist = (cursor->data).m_dist;
        bucket[i] = btmp;
        list_del(&cursor->mylist);
        // kfree(&cursor);
        i++;
    }

    ret = sprintf(buf, 
            "S/n\t\tTimestamp\t\tDuration\t\tMeasured\n"
            "[1]\t\t%lu\t\t%dus\t\t%dcm\n"
            "[2]\t\t%lu\t\t%dus\t\t%dcm\n"
            "[3]\t\t%lu\t\t%dus\t\t%dcm\n"
            "[4]\t\t%lu\t\t%dus\t\t%dcm\n"
            "[5]\t\t%lu\t\t%dus\t\t%dcm\n",
            bucket[0].t_stamp, bucket[0].t_high, bucket[0].m_dist,
            bucket[1].t_stamp, bucket[1].t_high, bucket[1].m_dist,
            bucket[2].t_stamp, bucket[2].t_high, bucket[2].m_dist,
            bucket[3].t_stamp, bucket[3].t_high, bucket[3].m_dist,
            bucket[4].t_stamp, bucket[4].t_high, bucket[4].m_dist
    );

    list_busy = 0;

    return ret;
}

ssize_t hcsr04_sysfs_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    return -1;      // write(), echo, etc. not supported;
}


/*
* vfs definitions below 
*/

int hcsr04_open(struct inode *inode, struct file *file) 
{   
    struct hcsr04_lifo_node *cursor, *tmp;
    int i;
    pending_read = 0;
    number_of_nodes = 0;

    list_for_each_entry_safe(cursor, tmp, &head_node, mylist) {
        if (cursor == NULL) break;
        list_del(&cursor->mylist);
        kfree(&cursor);
    }

    bucket_data.t_stamp = 0,
    bucket_data.t_high = 0,
    bucket_data.m_dist = 0

    for (i = 0; i < USER_mrq; i++) bucket[i] = bucket_data;

    return 0;
} 

int hcsr04_release(struct inode *inode, struct file *file) 
{
    return 0;
}

ssize_t hcsr04_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos) 
{   
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
                    getnstimeofday(&present_timestamp);
                    usd.t_stamp = (unsigned long)present_timestamp.tv_sec;    
                    usd.t_high = (unsigned int)ktime_to_ns(tmp_ktime);

                    while (list_busy);

                    if ((tmp_node = kmalloc(sizeof(struct hcsr04_lifo_node), GFP_KERNEL)) != NULL) {
                        (tmp_node->data).t_stamp = usd.t_stamp;
                        (tmp_node->data).t_high = usd.t_high;
                        (tmp_node->data).m_dist = (unsigned int)((usd.t_high/1000)/CNST_div);
                        // INIT_LIST_HEAD(&tmp_node->mylist);
                        list_busy = 1;
                        if (number_of_nodes < USER_mrq) {
                            list_add(&tmp_node->mylist, &head_node);
                            printk(KERN_INFO "New value added to list\n");
                            number_of_nodes++;
                        }
                        else if (number_of_nodes >= USER_mrq) {
                            lnode = list_first_entry(&head_node, struct hcsr04_lifo_node, mylist);
                            if (lnode == NULL)
                                return 0;
                            list_del(&lnode->mylist);
                            printk(KERN_INFO "Entry removed from list\n");
                            kfree(lnode);
                            list_add(&tmp_node->mylist, &head_node);
                        }
                        else {
                            kfree(tmp_node);
                        }
                        list_busy = 0;
                    }

                    return 0;
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
    int i = 0;

    if ((errn = alloc_chrdev_region(&hcsr04_devt, 0, 1, "hcsr04")) < 0) return errn;
    cdev_init(&hcsr04_cdev, &hcsr04_fops);
    hcsr04_cdev.owner = THIS_MODULE;
    
    errn = cdev_add(&hcsr04_cdev, hcsr04_devt, 1);
    if (errn) {
        printk(KERN_WARNING "Failed to add module\n");
        return errn;
    }

    hcsr04_device_root = root_device_register(hcsr04_rootname);
    if (IS_ERR(hcsr04_device_root)) {
        errn = PTR_ERR(hcsr04_device_root);
        printk(KERN_WARNING "Failed to create entry in /sys/device\n");
        return errn;
    }

    hcsr04_class = class_create(THIS_MODULE, hcsr04_name);
    if (IS_ERR(hcsr04_class)) {
        printk(KERN_WARNING "Failed to create class\n");
        errn = PTR_ERR(hcsr04_class);
        return errn;
    }

    hcsr04_device = device_create(hcsr04_class, hcsr04_device_root, hcsr04_devt, NULL, hcsr04_name);
    if (IS_ERR(hcsr04_device)) {
        errn = PTR_ERR(hcsr04_device);
        printk(KERN_WARNING "Failed to create device\n");
        return errn;
    }

    if ((errn = device_create_file(hcsr04_device, &last5)) < 0) {
        printk(KERN_WARNING "Failed to create entry in /sys/device/%s\n", hcsr04_name);
        return errn;
    }

    if ((errn = gpio_request(TRIG_pin, "Trigger pin")) < 0 ) return errn;
    if ((errn = gpio_request(ECHO_pin, "Echo pin")) < 0 ) return errn;
    if ((errn = gpio_direction_input(ECHO_pin)) < 0 ) return errn;
    if ((errn = gpio_direction_output(TRIG_pin, 0)) < 0 ) return errn;
    
    TRIG_timeout = ktime_set(0, ECHO_tmo); // Timeout = 0 second + ECHO_tms nanoseconds

    INIT_LIST_HEAD(&head_node);

    printk(KERN_INFO "%s loaded.\n", format_dev_t(buffer, hcsr04_devt));
    return 0;
}

static void __exit hcsr04_exit(void)
{
    gpio_free(ECHO_pin);
    gpio_free(TRIG_pin);
    list_for_each_entry_safe(lnode, tmp_node, &head_node, mylist) {
        list_del(&lnode->mylist);
        kfree(&lnode);
    }
    device_remove_file(hcsr04_device, &last5);
    device_destroy(hcsr04_class, hcsr04_devt);
    class_destroy (hcsr04_class);
    root_device_unregister(hcsr04_device_root);
    cdev_del(&hcsr04_cdev);
    unregister_chrdev_region(hcsr04_devt, 1);
}

module_init(hcsr04_init);
module_exit(hcsr04_exit);
MODULE_AUTHOR("Eyitope Adelowo");
MODULE_LICENSE("GPL");
