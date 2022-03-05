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
struct class *hcsr04_class;
struct device *hcsr04_device;
struct device *hcsr04_device_root;

static int bx;
ktime_t ECHO_high_starttime;
ktime_t ECHO_high_stooptime;
ktime_t tmp_ktime;
ktime_t TRIG_timeout;
static struct hcsr04_data usd;                     // user space data

static struct hcsr04_sysfs_data bucket[USER_mrq];

static struct hcsr04_sysfs_data ldata = {
    .t_high = 0,
    .m_dist = 0,
};

// struct sysfs_last_five_readings {
//     struct attribute attr;
//     ssize_t (*show)(struct kobject *kobj, struct kobj_attribute *attr, char *buf);
//     ssize_t (*store)(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count);
// };

struct device_attribute last5 = {
    .attr = {
        .name = "last5readings",
        .mode = 0666
    },
    .show = hcsr04_sysfs_show,
    .store = hcsr04_sysfs_store
};

// struct list_head giant_baby_head;
// static LIST_HEAD(giant_baby_head);

// struct list_data {
//     struct hcsr04_sysfs_data data;
//     struct list_head giant_baby_head;
// } *ldata;

/* ------------------------------------------*/


/*
* sysfs definitions below
*/

ssize_t hcsr04_sysfs_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    ssize_t ret = 0;

    // list_for_each_safe(i, tmp, &lifo_head) 
    // { 
    //     usd = list_entry(i, struct user_data, giant_baby_head);
    //     if (usd == NULL) tmpbuff[counter++] = emptyd;
    //     else tmpbuff[counter++] = usd->data;      
    //     list_del(i);
    //     kfree(usd);    
    //     if ((counter * sizeof(struct hcsr04_data)) == count) break;
    // }  

    ret = sprintf(buf, 
            "s/n\t\t\tDuration\t\t\tMeasured\n"
            "[1]\t\t\t%dus\t\t\t%dcm\n"
            "[2]\t\t\t%dus\t\t\t%dcm\n"
            "[3]\t\t\t%dus\t\t\t%dcm\n"
            "[4]\t\t\t%dus\t\t\t%dcm\n"
            "[5]\t\t\t%dus\t\t\t%dcm\n",
            bucket[0].t_high, bucket[0].m_dist,
            bucket[1].t_high, bucket[1].m_dist,
            bucket[2].t_high, bucket[2].m_dist,
            bucket[3].t_high, bucket[3].m_dist,
            bucket[4].t_high, bucket[4].m_dist
    );

    ldata.t_high = 0;
    ldata.m_dist = 0;
    for (bx = 0; bx < USER_mrq; bx++) bucket[bx] = ldata;
    bx = 0;

    return ret;
}

ssize_t hcsr04_sysfs_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    return -1;      // write(), echo, etc. not supported;
}


/*
* vfsfs definitions below 
*/

int hcsr04_open(struct inode *inode, struct file *file) 
{   
    pending_read = 0;
    bx = 0;
    return 0;
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
             
                    // if ((ldata = kmalloc(sizeof(struct list_data), GFP_KERNEL)) != NULL) {
                    //     (ldata->data).t_high = (unsigned int)(usd.t_high/1000);
                    //     (ldata->data).m_dist = (unsigned int)((usd.t_high/1000)/CNST_div);
                    //     list_add_tail(&ldata->giant_baby_head, )
                    // }

                    if (bx < USER_mrq) {
                        ldata.t_high = (unsigned int)(usd.t_high/1000);
                        ldata.m_dist = (unsigned int)((usd.t_high/1000)/CNST_div);
                        bucket[bx++] = ldata;
                        return 0;
                    }
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

    if ((errn = alloc_chrdev_region(&hcsr04_devt, 0, 1, "hcsr04")) < 0) return errn;
    cdev_init(&hcsr04_cdev, &hcsr04_fops);
    hcsr04_cdev.owner = THIS_MODULE;
    
    errn = cdev_add(&hcsr04_cdev, hcsr04_devt, 1);
    if (errn) {
        printk(KERN_WARNING "Failed to add module\n");
        return errn;
    }

    hcsr04_device_root = root_device_register(hcsr04_name);
    if (IS_ERR(hcsr04_device)) {
        errn = PTR_ERR(hcsr04_device);
        printk(KERN_WARNING "Failed to create entry in /sys/device\n");
        return errn;
    }

    // if ((errn = device_create_file(hcsr04_device, &last5)) < 0) {
    //     printk(KERN_WARNING "Failed to create entry in /sys/device/%s\n", hcsr04_name);
    //     return errn;
    // }

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

    if ((errn = gpio_request(TRIG_pin, "Trigger pin")) < 0 ) return errn;
    if ((errn = gpio_request(ECHO_pin, "Echo pin")) < 0 ) return errn;
    if ((errn = gpio_direction_input(ECHO_pin)) < 0 ) return errn;
    if ((errn = gpio_direction_output(TRIG_pin, 0)) < 0 ) return errn;
    
    TRIG_timeout = ktime_set(0, ECHO_tmo); // Timeout = 0 second + ECHO_tms nanoseconds
    
    /* Put bucket array in known state */
    ldata.t_high = 0;
    ldata.m_dist = 0;
    for (bx = 0; bx < USER_mrq; bx++) bucket[bx] = ldata;
    bx = 0;

    printk(KERN_INFO "%s loaded.\n", format_dev_t(buffer, hcsr04_devt));
    return 0;
}

static void __exit hcsr04_exit(void)
{
    gpio_free(ECHO_pin);
    gpio_free(TRIG_pin);
    device_destroy(hcsr04_class, hcsr04_devt);
    class_destroy (hcsr04_class);
    // device_remove_file(hcsr04_device, &last5);
    root_device_unregister(hcsr04_device_root);
    cdev_del(&hcsr04_cdev);
    unregister_chrdev_region(hcsr04_devt, 1);
}


module_init(hcsr04_init);
module_exit(hcsr04_exit);
MODULE_AUTHOR("Eyitope Adelowo");
MODULE_LICENSE("GPL");
