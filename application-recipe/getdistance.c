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

/*
 * Minimum time between read and write to device in microseconds.  
*/
#define READ_INTERVAL 6300
#define DIVISION_CONSTANT 58

struct hcsr04_data 
{
    unsigned long t_stamp;
    unsigned int t_high;
};

const char devname[] = "/dev/hcsr04";
struct hcsr04_data rcvd;
size_t bread = 0;


unsigned long millis()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (unsigned long)((tv.tv_sec) * 1000 + (tv.tv_usec) / 1000);
}

void clearscreen(void)
{
    int i;
    printf("\033[2J");        /*  clear the screen  */
    printf("\033[H");         /*  position cursor at top-left corner */
    for (i=1; i<=100; i++) {
        fflush(stdout);
        printf(i < 100 ? "\033[H" : "\n");
    }
}

int main(int argc, char *argv[])
{

    if (argc < 2) {
        printf("You must specify number of pings. Quitting.\n");
        printf("Usage: /usr/bin/getdistance <number_of_pings>\n");

        return -1;
    }

    int iter, cc, fd;     // Get from *argv[] and convert to int
    unsigned long duration = 0;
    double distance = 0.0;
    time_t ltime;

    if((cc = atoi(argv[1])) == 0) {
        printf("Invalid argument. Exiting...");
        exit(EXIT_FAILURE);
    }

    iter = cc;

    printf(" *** Test application for HC-SR04 driver ***\n");
    time(&ltime);
    printf("%s\n", ctime(&ltime));
    sleep(1);

    if ((fd = open(devname, O_RDWR)) < 0) {
        perror("Exiting...");
        exit(EXIT_FAILURE);
    }
    
    printf("/dev/hcsr04 opened\n");

    while(iter--)
    {
        write(fd, "1", 2);
        if ((bread = read(fd, &rcvd, sizeof(struct hcsr04_data)) < 0))
        {
            perror("read: FAIL");
            printf("Bytes read: %d\n", bread);
            continue;
        }
        printf("\r-------------------------------------\n"
                "                                       \n"
                "Ping:\t\t%i of %i                      \n"
                "Pulse duration:\t\t%d us               \n"
                "Distance:\t\t%.1f cm                   \n",
        (cc - iter), cc, (int)(rcvd.t_high/1000), (double)((rcvd.t_high/1000)/DIVISION_CONSTANT));
        sleep(1);
        clearscreen();
    }

    time(&ltime);
    close(fd);
    printf("\nEnd of test: %s\n", ctime(&ltime));

    return 0;
}