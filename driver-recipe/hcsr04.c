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
#include <linux/kdev_t.h>
#include <linux/fs.h>

static dev_t major_number;

// MAJOR(dev_t dev);
// MINOR(dev_t dev);
// MKDEV(major_num, minor_num);