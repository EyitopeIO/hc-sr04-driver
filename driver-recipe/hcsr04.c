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
#include <linux/delay.h>
#include <asm/uaccess.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/string.h>

#include "hcsr04.h"

#define TRIG_pin 23             // hcsr04 trigger pin
#define ECHO_pin 24             // hcsr04 echo pin
#define TRIG_tme 10             // hcsr04 trigger period
#define PLSE_tme 200            // Time for 8 pulses at 40kHz
#define SPEED_OF_SOUND 340      // In metres per second
#define MICRO 0.000001          // Micro multiplier
#define MAX_READS 5             

static dev_t hcsr04_devt;
struct cdev hcsr04_cdev; 
static char buffer[64];

/*
* State variables
*/
volatile int waiting_for_echo = 0;   
volatile int ECHO_pin_current_state = 0;
volatile ktime_t ECHO_high_starttime = 0;

/*
* Unit of hcsr04 data to add to the LIFO
*/
struct user_data {
    struct hcsr04_data data;
    struct list_head giant_baby_head;
};

/*
* Temporary working variables
*/
volatile struct user_data *gbp = NULL;

/*
* unsigned long long unsigned long long distance;
* Initialise the LIFO used to hold last 5 readings
*/
LIST_HEAD(lifo_head);

/*
* Take full control of GPIO 23 & 24 by configuring for input (echo) and output (trigger),
* and define interrupt for input
*/
int hcsr04_open(struct inode *inode, struct file *file) 
{   
    int error_n = 0;
    if ((error_n = gpio_request(TRIG_pin, "Trigger pin")) < 0 ) return error_n;
    if ((error_n = gpio_request(ECHO_pin, "Echo pin")) < 0 ) return error_n;
    if ((error_n = gpio_direction_input(ECHO_pin)) < 0 ) return error_n;
    if ((error_n = gpio_direction_output(TRIG_pin, 0)) < 0 ) return error_n;
    return 0;
}

int hcsr04_release(struct inode *inode, struct file *file) 
{
    gpio_free(ECHO_pin);
    gpio_free(TRIG_pin);
    free_irq(gpio_to_irq(ECHO_pin), NULL);
    return 0;
}

ssize_t hcsr04_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos) 
{
    struct hcsr04_data tmpbuff[MAX_READS];
    struct hcsr04_data emptyd = {   // Represents a NULL or unavailable data.
        .t_stamp = 0,
        .t_high = 0,
    };   
    struct list_head *i, *tmp;
    struct user_data *usd;
    size_t counter = 0;
    unsigned long not_copied = 0;
    
    if (count == 0) return 0;
    
    list_for_each_safe(i, tmp, &lifo_head) 
    { 
        usd = list_entry(i, struct user_data, giant_baby_head);
        if (usd == NULL) tmpbuff[counter++] = emptyd;
        else tmpbuff[counter++] = usd->data;      
        list_del(i);
        kfree(usd);    
        if (counter == count) break;
    }  
    not_copied = copy_to_user(buf, tmpbuff, sizeof(tmpbuff)*counter);
    return sizeof(tmpbuff) * counter;   
}

ssize_t hcsr04_write(struct file *filp, const  char *buffer, size_t length, loff_t * offset)
{
    size_t le = strlen(buffer);
    if (le < 0) return -1;
    
    gpio_set_value(TRIG_pin, 1);
    udelay(TRIG_tme);   // Wait 10 micro seconds
    gpio_set_value(TRIG_pin, 0);
    
    while (!gpio_get_value(ECHO_pin));  // Wait for pin to go high
    ECHO_high_starttime = ktime_get_real_ns();
    while (gpio_get_value(ECHO_pin));  // Wait for pin to go low
    
    if ((gbp = kmalloc(sizeof(struct user_data), GFP_KERNEL)) != NULL)
    {
        (gbp->data).t_stamp = (unsigned long)ktime_get_real();
        (gbp->data).t_high = (int)(ktime_get_real_ns() - ECHO_high_starttime);
        list_add(&gbp->giant_baby_head, &lifo_head);
        return 0;
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
    alloc_chrdev_region(&hcsr04_devt, 0, 1, "hcsr04");
    printk(KERN_INFO "%s loaded.\n", format_dev_t(buffer, hcsr04_devt));
    cdev_init(&hcsr04_cdev, &hcsr04_fops);
    hcsr04_cdev.owner = THIS_MODULE;
    cdev_add(&hcsr04_cdev, hcsr04_devt, 1);
    return 0;
}

static void __exit hcsr04_exit(void)
{
    cdev_del(&hcsr04_cdev);
    unregister_chrdev_region(hcsr04_devt, 1);
}

module_init(hcsr04_init);
module_exit(hcsr04_exit);
MODULE_AUTHOR("Eyitope Adelowo");
MODULE_LICENSE("GPL");
