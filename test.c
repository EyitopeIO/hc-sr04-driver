/*
* Eyitope Adelowo (adelowoe@coventry.ac.uk)
* 17-FEB-2022
* 
* 
*/


#include <unistd.h>
#include <time.h>
#include <math.h>
#include <sys/time.h>
#include <stdio.h>

#define READ_INTERVAL 6300

unsigned long millis()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (unsigned long)((tv.tv_sec) * 1000 + (tv.tv_usec) / 1000);
}


int main(int argc, char *argv[])
{
    unsigned count = 7;     // Get from *argv[] and convert to int
    unsigned long duration = 0;
    unsigned int iter = count;
    double distance = 0.0;

    time_t ltime;
    time(&ltime);

    printf("Test application for HC-SR04 driver\n");
    printf("Start of test: %s", ctime(&ltime));   // Get ctime here

    while(iter--)
    {
        duration = millis();
        // Send echo pulse here
        usleep(READ_INTERVAL);
        // Read pulse here
        duration = millis() - duration;

        printf (
            "\r--------------------------------------"
            "                                       "
            "Pulse     %i                           "
            "Duration: %d s                         "
            "Distance: %d cm                        "
            "                                       "
            "---------------------------------------",
        abs(count - iter), (unsigned int)duration, distance);

        sleep(1);
    }

    time(&ltime);
    ctime(&ltime);

    printf("\n");
    printf("End of test: %%s");

    return 0;
}