#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

// types
#include <stdbool.h>
#include <string.h>

#include "include/logger.h"
#include "include/main.h"
#include "include/utils.h"

void cmd_input()
{
    thread_logger *thl_cmdinput = new_thread_logger(debug_mode);
    //LOGF_DEBUG(thl_cmdinput, 0, "cmd input thread started", "printf");
    char *input_cmd;
    
    for (;;)
    {
        // LOGF_DEBUG(thl_cmdinput, 0 , "for ;; ran", "printf");
        usleep(40 * 1000);
        input_cmd = malloc((50 + 1) * sizeof(char)); // +1 for '\0' character

        int scanfret = scanf("%s", input_cmd);
        if (scanfret > 1)
        {
            LOGF_DEBUG(thl_cmdinput, 0, "Potential error, scanf ret %d", scanfret);
        }

        if (strcmp(input_cmd, "clearscr") == 0)
        {
            for (int i = 0; i <= 100; i++) {
                printf("\e[1;1H\e[2J"); // POSIX clearscreen
            }
            printf("\n");
        }

        else if (strcmp(input_cmd, "showthreads") == 0)
        {
            thread_store(update);
            if (thread_count - 2 < 0) {
                // LOGF_ERROR(thl_cmdinput, 0, "Thread count is below zero, something went wrong", "printf");
            }
            printf("Thread count : %d\n", thread_count); 
            printf("\n");
        }

        else if (strcmp(input_cmd, "showtimedips") == 0)
        {
            for (int i = 0; i <= timer_rawlen; i++)
            {
                printf("[%d] : ", i);
                printf("%s\n", timed_addresses[i]);
            }
            printf("-------------------------\n");
        }

        else if (strcmp(input_cmd, "showbans") == 0)
        {
            iparry_usedamount = update_used_ipaddr_elements();
            if (iparry_usedamount <= 0)
            {
                printf("No banned IP addresses\n");
                clear_thread_logger(thl_cmdinput);
                pthread_t input_thread;
                pthread_create(&input_thread, NULL, (void *)cmd_input, NULL);
                pthread_exit(0);
            }
            printf("-------------------------\n");

            for (int i = 0; i <= banned_rawlen; i++)
            {
                printf("[%d] : ", i);
                printf("%s\n", banned_addresses[i]);
            }
            printf("-------------------------\n");
        }

        else if ((strcmp(input_cmd, "ban") == 0) || (banning == true))
        {
            if (banning == true) // at this point, input_cmd equals the IP address the user gave us
            {
                bool already_stored = false;

                // to prevent making duplicates
                for (int i = 0; i <= banned_rawlen; i++)
                {
                    if (strcmp(banned_addresses[i], input_cmd) == 0)
                    {
                        printf("IP already in ban list, not banning\n");
                        already_stored = true;
                        banning = false;
                        break;
                    }
            
                    else if (strcmp(banned_addresses[i], input_cmd) != 0) {
                        // if doesn't match
                        continue;
                    }
                }

                iparry_usedamount = update_used_ipaddr_elements();
                if (iparry_usedamount >= banned_rawlen)
                { // if we're full
                    LOGF_ERROR(thl_cmdinput, 0, "ERROR : Banned list is full", "printf");
                    banning = false; // we aint banning anymore
                }
            
                if (already_stored == false)
                {
                    // append to array
                    for (int i = 0; i <= banned_rawlen; i++)
                    {
                        if (strcmp(banned_addresses[i], "") == 0)
                        { // if is blank
                            strncpy(banned_addresses[i], input_cmd, sizeof(banned_addresses[i]));
                            LOGF_INFO(thl_cmdinput,0,"Banned IP address %s",banned_addresses[i]);

                            banning = false;
                            break;
                        }
            
                        else if (strcmp(banned_addresses[i], "") != 0) {
                            // if the element is occupied
                            continue; // do nothing, skip to the next iteration.
                        }
                    }
                }
            }

            if (strcmp(input_cmd, "ban") == 0) { banning = true; }
        }

        else if ((strcmp(input_cmd, "unban") == 0) || (unbanning == true))
        {
            if (unbanning == true) // once input_cmd contains the ip address
            {
                for (int i = 0; i <= banned_rawlen; i++)
                {
                    if (strcmp(banned_addresses[i], input_cmd) == 0)
                    {
                        printf("Found IP address in ban list, removing from list\n");
                        strncpy(banned_addresses[i], "", 1);
                        break;
                    }
            
                    else if (strcmp(banned_addresses[i], input_cmd) != 0)
                    { // if doesn't match input IP address
                        continue;
                    }
                }
                unbanning = false;
            }
            if (strcmp(input_cmd, "unban") == 0) { unbanning = true; }
        }

        else if ((strcmp(input_cmd, "exit") == 0))
        {
            int delfile = system("rm -rf /dev/shm/linkup-varstore/thread_count");
            if (delfile <= -1) {
                LOGF_ERROR(thl_cmdinput, 0, "Failed to remove file, %s (Error code %d): ", strerror(errno), errno);
            }

            int deldir = remove("/dev/shm/linkup-varstore/");
            if (deldir == -1) {
                LOGF_ERROR(thl_cmdinput, 0, "Failed to remove dir, %s (Error code %d): ", strerror(errno), errno);
            }
            exit(0);
        }

        else if (strcmp(input_cmd, "help") == 0 || strcmp(input_cmd, "cmds") == 0)
        {
            showhelp();
        }

        else
        {
            printf("Command not recognized\n");
            printf("Attempted command : %s\n", input_cmd);
        }

        memset(input_cmd,0,lengthofstring(input_cmd) + 1); // +1 for null terminator
    }

    free(input_cmd);
    clear_thread_logger(thl_cmdinput);
    pthread_exit(0);
}