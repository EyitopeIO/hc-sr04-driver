/*
* Eyitope Adelowo (adeyitope.io@gmail.com)
* 20-FEB-2022
*
* -------------
* DESCRIPTION
* -------------
* Header file for user to use the hcsr04 driver.
*/



/*
 * Data structure to get a unit of data from the driver.
 *
 * t_stamp:      Unix time when the device was triggered.
 * t_high:       Echo pulse duration
 * m_dist:       Measured distance
*/

struct hcsr04_data
{
    unsigned long t_stamp;
    unsigned int t_high;
};

struct hcsr04_sysfs_ldata
{
    unsigned long t_stamp;
    unsigned int t_high;
    unsigned int m_dist;
};
