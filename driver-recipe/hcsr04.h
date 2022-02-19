/*
* Eyitope Adelowo (adelowoe@coventry.ac.uk)
* 20-FEB-2022
*
* -------------
* DESCRIPTION
* -------------
* Header file for user to use the hcsr04 driver.
*
*/



/*
 * Data structure to get a unit of data from the driver.
 *
 * tstamp:      Unix time when the device was triggered.
 * techo:       Time between trigger and the echo.
 * distance:    Measured distance     
*/
struct hcsr04_data 
{
    unsigned long t_stamp;
    unsigned int t_echo;
    float distance;
};