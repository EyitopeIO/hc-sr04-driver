/*
*
* Eyitope Adelowo (adelowoe@coventry.ac.uk)
* 17-FEB-2022
* 
* Developed during 7041CEM, Coventry University 
*/


#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <sys/time.h>
#include <fcntl.h>
#include <stdio.h>
// #include <linux/hcsr04.h>

/*
 * Minimum time between read and write to device in microseconds.  
*/
#define READ_INTERVAL 6300


unsigned long millis()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (unsigned long)((tv.tv_sec) * 1000 + (tv.tv_usec) / 1000);
}


int main(int argc, char *argv[])
{
    unsigned iter = 7;     // Get from *argv[] and convert to int
    unsigned long duration = 0;
    double distance = 0.0;
    int fd = -1;
    const char devname = "/dev/hcsr04";
    time_t ltime;

    printf(" *** Test application for HC-SR04 driver ***\n");
    sleep(1);

    time(&ltime);
    printf("Start of test: %s\n", ctime(&ltime));   // Get ctime here

    if ( (fd = open(devname, O_RDWR)) < 0) {
        perror(NULL);
        exit(EXIT_FAILURE);
    }

    while(iter--)
    {
        duration = millis();
        // Send echo pulse here by write() to device.
        // Block here with read() until data available.
        duration = millis() - duration;

        printf (
            "\r-------------------------------------"
            "                                       "
            "count:\t\t%i                           "
            "Duration:\t\t%d s                      "
            "Distance:\t\t%d cm                     "
            "--------------------------------------",
        iter, (unsigned int)duration, distance);

        sleep(1);
    }

    time(&ltime);
    printf("\nEnd of test: %s\n", ctime(&ltime));

    return 0;
}