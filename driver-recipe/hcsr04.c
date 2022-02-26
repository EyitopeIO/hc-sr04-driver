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
#include ,linux/slab.h>
#include <linux/types.h>.
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <asm/uaccess.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>

#include <hcsr04.h>

#define TRIG_pin 23     // hcsr04 trigger pin
#define ECHO_pin 24     // hcsr04 echo pin

static dev_t hcsr04_devt;
static cdev hcsr04_cdev; 



/*
* State variables
*/
unsigned char waiting_for_echo = 0;   
unsigned int ECHO_pin_current_state = 0;  

/*
* LIFO to hold the most recent 5 measured distance
*/
static struct user_data{
    struct hcsr04_data;
    struct list_head giant_baby_head;
};

LIST_HEAD(lifo_head);

struct file_operations hcsr04_fops = {
    .owner = THIS_MODULE;
    .open = hcsr04_open;
    .read = hcsr04_read;
    .write = hcsr04_write;
    .release = hcsr04_release;
}

static irq_handler_r InterruptHandler_ECHO_pin_change(unsigned int irq, struct pt_regs *regs)
{
    ECHO_pin_current_state = gpio_get_value(ECHO_pin);
    if (waiting_for_echo && ECHO_pin_current_state)
    {
        // Get start timestamp here
    }
    else if (waiting_for_echo && !ECHO_pin_current_state)
    {
        // Get end timestamp here
        waiting_for_echo = 0;
    }
    return (irq_handler_t)IRQ_HANDLED;
}

static int __init hcsr04_init(void) 
{
    alloc_chrdev_region(&hcsr04_devt, 0, 1, "hcsr04");
    cdev_init(&hcsr04_cdev, &hcsr04_fops);
    hcsr04_cdev.owner = THIS_MODULE;
    cdev_add(&hcsr04_cdev, hcsr04_devt, 1);
    return 0;
}

/*
* Take full control of GPIO 23 & 24 by configuring for input (echo) and output (trigger),
* and define interrupt for input
*/
int hcsr04_open(struct inode *inode, struct file *file)
{   
    int error_n = 0;
    if ((error_n = gpio_request(TRIG_pin, "Trigger pin")) < 0 ) return errno;
    if ((error_n = gpio_request(ECHO_pin, "Echo pin")) < 0 ) return errno;
    if ((error_n = gpio_direction_input(ECHO_pin, "Echo pin")) < 0 ) return errno;
    if ((error_n = gpio_direction_output(TRIG_pin, "Echo pin")) < 0 ) return errno;
    
    if ((error_n = request_irq(gpio_to_irq(ECHO_pin), (irq_handler_t)InterruptHandler_ECHO_pin_change, IRQF_TRIGGER_RISING | IRQ_TRIGGER_FALLING), "Interrupt request", NULL) < 0 )
        return errno;
}

/*
* Free all GPIO resources obtained
*/
int hcsr04_release(struct inode *inode, struct file *file)
{
    gpio_free(ECHO_pin);
    gpio_free(TRIG_pin);
    free_irq(gpio_to_irq(ECHO_pin), NULL);
}

/*
* Return the hcsr04 data structure to the user
*/
ssize_t hcsr04_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    
    hcsr04_data tmpbuff[count];
    hcsr04 emptyd = {   // Represents a NULL or unavailable data.
        .t_stamp = 0;
        .t_echo = 0;
        .distance = 0.0;
    }
    
    struct list_head *i, *tmp;
    struct user_data *usd;
    size_t counter = 0;
    
    if (count == 0) return 0;
    
    list_for_each_safe(i, tmp, &lifo_head) {
        
        usd = list_entry(i, struct user_data, giant_baby_head);
        
        if (usd == NULL) tmpbuff[counter++] = emptyd;
        else tmpbuff[counter++] = *usd;
        
        list_del(i);
        kfree(usd);
        
        if (counter == count) break;
    }
    
    copy_to_user(buf, tmpbuff, sizeof(tmpbuff));
    return sizeof(tmpbuff);
    
}

// Define write() here
// Receives a string of length 2 i.e. character with null
// You pull the trigger, but reading won't be possible until data is available


static void __exit hcsr04_exit(void)
{
    cdev_del(&hcsr04_cdev);
    unregister_chrdev_region(hcsr04_devt, 1);
}

module_init(hcsr04_init);
module_exit(hcsr04_exit);
MODULE_AUTHOR("Eyitope Adelowo");
MODULE_LICENSE("MIT");
