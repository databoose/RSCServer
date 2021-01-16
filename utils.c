#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

// netutils
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>

// types
#include <stdbool.h>
#include <string.h>

#include "include/logger.h"
#include "include/main.h"
#include "include/utils.h"
#include "include/session_info.h"

// net utils

void safesend(int * clisock, int TID, char *buf)
{
    if (strchr(buf,'\n') == NULL) {
        LOGF_ERROR(global_thl, 0, "safesend : no newline detected in buffer provded, emergency closing socket", "printf");
        close(*clisock);
        pthread_exit(0);
    }

    int send_status = send(*clisock, (void *)buf, (size_t)lengthofstring(buf), 0);
    if (send_status == -1) {
        print_send_err(TID);
        if (errno == 32) { // if SIGPIPE
            LOGF_ERROR(global_thl, 0, "Broken pipe detected. (TID %d)", TID, "printf");
        }
        LOGF_DEBUG(global_thl, 0, "Dropping thread", "printf");
        close(*clisock);
        pthread_exit(0);
    }
    else {
        LOGF_DEBUG(global_thl, 0 , "Sent %s", buf, "printf");
    }
}

char *saferecv(int * clisock, int TID, size_t len, char* type, char *expected_string)
{
    char *buf = malloc((len + 1) * sizeof(char)); // +1 for '\0' character
    
    LOGF_DEBUG(global_thl, 0, "Waiting for %s message from client ... ", type, "printf");
    int recv_status = recv(*clisock, (void *)buf, (len + 1), 0);
    if (recv_status == -1)
    {
        // we failed recv, so print and return SOCKET_ERROR for handle_connection to deal
        print_recv_err(TID);
        strncpy(buf, "SOCKET_ERROR", 13);
        return buf;
        // close(*clisock);
        // pthread_exit(0);
    }

    if (strcmp(expected_string, NULLSTRING) != 0) // if something is passed here
    {
        if (strcmp(expected_string, buf) == 0)
        {
            LOGF_DEBUG(global_thl, 0, "Expected message lines up with received message \"%s\"", buf, "printf");
        }
        else
        {
            LOGF_ERROR(global_thl, 0, "String mismatch, expected string does not match up with message \"%s\" from client...", buf, "printf");
            LOGF_DEBUG(global_thl, 0 , "Exiting thread due to verification failure", "printf");
            
            close(*clisock);
            pthread_exit(0);
        }
    }

    return buf;
}

// signal handling 

void sig_handler(int signo)
{
    if (signo == SIGINT) // control+c
    {
        printf("\nInterrupt request detected, exiting...\n");
        int delfile = system("rm -rf /dev/shm/linkup-varstore/thread_count");
        if (delfile <= -1) { 
            LOGF_ERROR(global_thl, 0, "Failed to remove file, %s (Error code %d): ", strerror(errno), errno); 
        }

        int deldir = remove("/dev/shm/linkup-varstore/");
        if (deldir == -1) { 
            LOGF_ERROR(global_thl, 0, "Failed to remove dir, %s (Error code %d): ", strerror(errno), errno); 
        }
        exit(0);
    }

    if (signo == SIGSEGV) // segfault
    {
        LOGF_ERROR(global_thl, 0, "Segfault caught, exiting...", "printf");
        int delfile = system("rm -rf /dev/shm/linkup-varstore/thread_count");
        if (delfile <= -1) { 
            LOGF_ERROR(global_thl, 0, "Failed to remove file, %s (Error code %d): ", strerror(errno), errno); 
        }

        int deldir = remove("/dev/shm/linkup-varstore/");
        if (deldir == -1) { 
            LOGF_ERROR(global_thl, 0, "Failed to remove dir, %s (Error code %d): ", strerror(errno), errno); 
        }
        exit(0);
    }
}

// basic utils

char *strremove(char *str, const char *sub) 
{
    char *p, *q, *r;
    if ((q = r = strstr(str, sub)) != NULL) {
        size_t len = strlen(sub);
        while ((r = strstr(p = r + len, sub)) != NULL) {
            while (p < r)
                *q++ = *p++;
        }
        while ((*q++ = *p++) != '\0')
            continue;
    }
    return str;
}

void priter(char* tempstring)
{
    printf("PRITER : -START-");
    
    for (int i=0; tempstring[i]; i++) {
      printf("%c",tempstring[i]);
    }
    printf("-EOC-\n");

    if (strstr(tempstring, "\0") != NULL) {
        printf("nullterm detected\n");
        // contains
    }
    if (strstr(tempstring, "\n") != NULL) {
        printf("newline detected\n");
        // contains
    }
}

int lengthofstring(char* tempstring)
{
    int charcount = 0;
    for (int i=0; tempstring[i]; i++) 
    {
        if (tempstring[i] != ' ') {
            charcount++;
        }
    }
    return charcount;
}

void print_recv_err(int TID)
{
    LOGF_ERROR(global_thl, 0, "Error reading from socket : (TID : %ld)", TID);
    LOGF_ERROR(global_thl, 0, "%s (Error code %d)", strerror(errno), errno);
}

void print_send_err(int TID)
{
    LOGF_ERROR(global_thl, 0, "Error writing to socket : (TID : %ld)\n", TID);
    LOGF_ERROR(global_thl, 0, "%s (Error code %d) \n", strerror(errno), errno)
    
    
}

void thread_store(enum THREAD_STORE_OPTION opt)
{
    FILE *fptr;

    if (opt == create)
    {
        LOGF_DEBUG(global_thl, 0, "Creating directory for shared memory storage", "printf");
        if (system("mkdir /dev/shm/linkup-varstore") <= -1)
        {
            printf("possible system() error : %s (Error code %d)\n", strerror(errno), errno);
        }
    }

    if (opt == update)
    {
        int delfile = system("rm -rf /dev/shm/linkup-varstore/thread_count");
        if (delfile <= -1) { LOGF_ERROR(global_thl, 0, "Failed to remove file, %s (Error code %d): ", strerror(errno), errno); }

        int deldir = remove("/dev/shm/linkup-varstore/");
        if (deldir == -1) { LOGF_ERROR(global_thl, 0, "Failed to remove dir, %s (Error code %d): ", strerror(errno), errno); }

        int sysmkdir = system("mkdir /dev/shm/linkup-varstore/");
        if (sysmkdir <= -1) { LOGF_ERROR(global_thl, 0, "possible system() mkdir error : %s (Error code %d)\n", strerror(errno), errno); }

        int sysappend = system(appendcmd);
        if (sysappend <= -1) { LOGF_ERROR(global_thl, 0, "possible syste() appendcmd error : %s (Error code %d)\n", strerror(errno), errno); }

        fptr = fopen("/dev/shm/linkup-varstore/thread_count", "r"); //fopen automatically creates this file for us if it doesn't exist, if entire directory does not exist however, we need to manually create the dir (which is why we call mkdir before)
        while (!feof(fptr))                                         // segfault if no file or directory
        {
            if (fscanf(fptr, "%d", &thread_count) == 0) {
                printf("possible fscaf error : %s (Error code %d)\n", strerror(errno), errno);
                
            }
        }

        thread_count = thread_count - timer_thread_count - 2; // minus two because we're not including cmd_input thread and main thread, 
                                                              // minus timer_thread_count because not including timer threads
        fclose(fptr);
        remove("/dev/shm/linkup-varstore/");
    }
}

void banlist_append_ipaddr(char *THREAD_IP)
{
    bool already_stored = false;

    // to prevent making duplicates
    for (int i = 0; i <= BANNED_RAWLEN_SIZE - 1; i++)
    {
        if (strcmp(banned_addresses[i], THREAD_IP) == 0)
        {
            printf("IP already in array, not appending\n");
            already_stored = true;
            break;
        }

        else if (strcmp(banned_addresses[i], THREAD_IP) != 0) {
            // if doesn't match
            continue;
        }
    }

    if (already_stored == false)
    {
        // append to array
        for (int i = 0; i <= BANNED_RAWLEN_SIZE - 1; i++)
        {
            if (strcmp(banned_addresses[i], "") == 0)
            { // if is blank
                strncpy(banned_addresses[i], THREAD_IP, sizeof(banned_addresses[i]));
                break;
            }

            else if (strcmp(banned_addresses[i], "") != 0) {
                // if the element is occupied
                continue; // do nothing, skip to the next iteration.
            }
        }
    }
}

void banlist_remove_ipaddr(char *THREAD_IP)
{
    for (int i = 0; i <= BANNED_RAWLEN_SIZE - 1; i++)
    {
        if (strcmp(banned_addresses[i], THREAD_IP) == 0)
        {
            LOGF_DEBUG(global_thl, 0 ,"IP address %s removed from banlist\n", THREAD_IP, "printf");
            strncpy(banned_addresses[i], "", 1);
            break;
        }

        else if (strcmp(banned_addresses[i], THREAD_IP) != 0)
        { // if doesn't match input IP address
            continue;
        }
    }
}

void timer_signal_ran(char *THREAD_IP) // tells timer thread that the client is talking to us and has ran something
{
    bool submitted = false;

    for (int i=0; i<=IPSIGNAL_RAWLEN_SIZE - 1; i++)
    {
        if (strcmp(signal_addresses[i],"") == 0) // if blank, let's populate this one
        {
            strncpy(signal_addresses[i], THREAD_IP, sizeof(signal_addresses[i]));
            //LOGF_DEBUG(global_thl, 0, "Putting IP in signal list", "printf");
            usleep(260 * 1000); // 260ms to wait for timer thread to register that we just ran
            strncpy(signal_addresses[i], "", sizeof(signal_addresses[i]));
            //LOGF_DEBUG(global_thl, 0, "Removing IP from signal list", "printf");

            submitted = true;
            break;
        }
    }

    if (submitted == false) { LOGF_ERROR(global_thl, 0, "Could not find an empty array element in ipsignal to popluate, fixme.", "printf"); }
}

void timer_append_ipaddr(char *THREAD_IP)
{
    bool already_stored = false;

    // to prevent making duplicates
    for (int i = 0; i <= TIMER_RAWLEN_SIZE - 1; i++)
    {
        if (strcmp(timed_addresses[i], THREAD_IP) == 0)
        {
            already_stored = true;
            break;
        }

        else if (strcmp(timed_addresses[i], THREAD_IP) != 0) {
            // if doesn't match
            continue;
        }
    }

    if (already_stored == false)
    {
        // append to array
        for (int i = 0; i <= TIMER_RAWLEN_SIZE - 1; i++)
        {
            if (strcmp(timed_addresses[i], "") == 0)
            { // if is blank
                strncpy(timed_addresses[i], THREAD_IP, sizeof(timed_addresses[i]));
                break;
            }

            else if (strcmp(timed_addresses[i], "") != 0) {
                // if the element is occupied
                continue; // do nothing, skip to the next iteration.
            }
        }
    }
}

void timer_remove_ipaddr(char *THREAD_IP)
{
    for (int i = 0; i <= TIMER_RAWLEN_SIZE - 1; i++)
    {
        if (strcmp(timed_addresses[i], THREAD_IP) == 0)
        {
            LOGF_DEBUG(global_thl, 0, "Removing %s from timer list\n", timed_addresses[i], "printf");
            strncpy(timed_addresses[i], "", 1);
            break;
        }

        else if (strcmp(timed_addresses[i], THREAD_IP) != 0)
        { // if doesn't match input IP address
            continue;
        }
    }
}