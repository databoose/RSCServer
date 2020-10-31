#include <stdio.h>
#include <stdlib.h>

// types
#include <stdbool.h>
#include <string.h>
#include <unistd.h> // sleep function
#include <errno.h>

#include "include/logger.h"
#include "include/main.h"
#include "include/utils.h"

void handle_timer(void *VPTR_THREAD_IP)
{
    thread_logger *thl_timer = new_thread_logger(debug_mode);
    char *THREAD_IP = (char *)VPTR_THREAD_IP;
    timer_thread_count++; // this is to disclude from general thread count

    for (int i = 0; i <= TIMER_RAWLEN_SIZE - 1; i++)
    {
        if (strcmp(timed_addresses[i], THREAD_IP) == 0)
        { // if IP is still in timed_address array
            // LOGF_DEBUG(thl_timer, 0 ,"Timer thread already found for this IP, exiting thread\n","printf");
            clear_thread_logger(thl_timer);
            timer_thread_count--;
            pthread_exit(0);
            break;
        }
    }

    timer_append_ipaddr(THREAD_IP);
    
    // not really needed to lock a mutex on this
    int TIMER_TID;
    FILE *urand_ptr;
    urand_ptr = fopen("/dev/urandom", "rb");
    if (fread(&TIMER_TID, 1, 3, urand_ptr) <= 0) { // read 3 bytes to fill int
        LOGF_ERROR(thl_timer, 0, "fread error, returned 0 or below", "printf");
        LOGF_ERROR(thl_timer, 0, "%s", strerror(errno), "printf");
    }
    if (TIMER_TID < 0) {
        TIMER_TID = abs(TIMER_TID);
    }
    fclose(urand_ptr);

    LOGF_DEBUG(thl_timer, 0, "TIMER TID : %d", TIMER_TID, NULL);

    struct timerblock block = {.seconds_passed = 0, .times_ran = 0};
    struct timerblock longer = {.seconds_passed = 0, .times_ran = 0};

    bool detected = false;

    do
    {
        usleep(500 * 1000); // sleep for half of a second
        block.seconds_passed = block.seconds_passed + 0.50;
        longer.seconds_passed = longer.seconds_passed + 0.50;

        // printf("block seconds : %.2f\n", block.seconds_passed);
        // printf("longer seconds passed : %.2f\n", longer.seconds_passed);

        detected = false;
        for (int i = 0; i <= TIMER_RAWLEN_SIZE - 1; i++)
        {
            if (strcmp(timed_addresses[i], THREAD_IP) == 0) 
            {
                detected = true;
                //printf("in\n");
                break; // break out of the for loop, but continues in do while loop
            }
        }

        for (int i=0; i<=IPSIGNAL_RAWLEN_SIZE - 1; i++)
        {
            // printf("[%d] : %s\n",i, signal_addresses[i]); debugging stuck elements
            if (strcmp(signal_addresses[i], THREAD_IP) == 0)
            {
                block.times_ran++;
                LOGF_DEBUG(thl_timer, 0, "Timer thread (%d) : incrementing block.times_ran : %d", TIMER_TID, block.times_ran, "printf");
                break;
            }
            
            else
            {
                //printf("i : %d\n", i);
                continue;
            }
        }

        if (block.seconds_passed >= 1) // wait for a second to pass or else don't run all of these subroutine checks
        {
            if (longer.seconds_passed >= TIMEOUT_IN) // if nothing ran after specified amount of seconds
            {
                if (longer.times_ran <= 0)
                {
                    break; // breaks out of do loop to exit the thread
                }

                else
                {
                    // reset block of time so we check again at specified amount of seconds
                    longer.seconds_passed = 0;
                }
            }

            if (block.seconds_passed >= 8)
            {
                if (block.times_ran >= 5)
                {
                    LOGF_INFO(thl_timer, 0, "Timing out %s for 30 seconds", THREAD_IP, "printf");
                    banlist_append_ipaddr(THREAD_IP);
                    sleep(30);
                    banlist_remove_ipaddr(THREAD_IP);
                }

                // always reset block values if seconds are equal to or above 8 no matter what
                block.seconds_passed = 0;
                block.times_ran = 0;
            }
        }
    }
    while (detected == true);

    printf("Exiting %s timer thread (TIMER TID : %d), no longer active\n", THREAD_IP, TIMER_TID);
    clear_thread_logger(thl_timer);
    timer_remove_ipaddr(THREAD_IP);
    timer_thread_count--;
    pthread_exit(0);
}