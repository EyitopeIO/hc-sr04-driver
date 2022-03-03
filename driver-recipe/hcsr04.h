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
 * t_stamp:      Unix time when the device was triggered.
 * t_echo:       Time between trigger and the echo.
 * t_high:       Measured distance     
*/
struct hcsr04_data 
{
    unsigned long t_stamp;
    unsigned int t_high;
};