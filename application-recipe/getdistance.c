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
#define DIVISION_CONSTANT 58

struct hcsr04_data 
{
    unsigned long long t_stamp;
    int t_high;
};


const char any[] = "any";
struct hcsr04_data rcvd;;
size_t bread = 0;


unsigned long millis()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (unsigned long)((tv.tv_sec) * 1000 + (tv.tv_usec) / 1000);
}


int main(int argc, char *argv[])
{
    unsigned iter = 7;     // Get from *argv[] and convert to int
    int cc = 7;
    unsigned long duration = 0;
    double distance = 0.0;
    int fd = 0;
    const char devname[] = "/dev/hcsr04";
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
        write(fd, any, sizeof(any));
        if ((bread = read(fd, &rcvd, sizeof(struct hcsr04_data) * 1)) < 0)
        {
            perror("read: FAIL");
            printf("Bytes read: %d\n", bread);
            continue;
        }

        printf (
            "\r-------------------------------------"
            "                                       "
            "count:\t\t%i                           "
            "Pulse duration:\t\t%d us               "
            "Distance:\t\t%.1f cm                   "
            "-------------------------------------- ",
        (cc - iter), rcvd.t_high, (rcvd.t_high/DIVISION_CONSTANT)
        );

        sleep(1);
    }

    time(&ltime);
    printf("\nEnd of test: %s\n", ctime(&ltime));

    return 0;
}