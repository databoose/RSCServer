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

// net utils

void safesend(int * clisock, int TID, thread_logger *logger, char *buf)
{
    if (strchr(buf,'\n') == NULL) {
        LOGF_ERROR(logger, 0, "safesend : no newline detected in buffer provded, emergency closing socket", "printf");
        close(*clisock);
        pthread_exit(0);
    }

    int send_status = send(*clisock, (void *)buf, (size_t)lengthofstring(buf), 0);
    if (send_status == -1) {
        print_send_err(TID);
        close(*clisock);
        pthread_exit(0);
    }
}

char *saferecv(int * clisock, int TID, thread_logger *logger, size_t len, char *expected_string)
{
    char *buf = malloc((len + 1) * sizeof(char)); // +1 for '\0' character
    
    LOGF_DEBUG(logger, 0, "Waiting for message from client ... ", "printf");
    int recv_status = recv(*clisock, (void *)buf, (len + 1), 0);
    if (recv_status == -1)
    {
        print_recv_err(TID);
        close(*clisock);
        pthread_exit(0);
    }

    if (expected_string != NULL)
    {
        if (strcmp(expected_string, buf) == 0)
        {
            LOGF_DEBUG(logger, 0, "Expected message lines up with received message (\"%s\")", buf, "printf");
        }

        else
        {
            LOGF_ERROR(logger, 0, "String mismatch, expected string does not match up with message from client...", "printf");
            LOGF_DEBUG(logger, 0 , "Exiting thread due to verification failure", "printf");
            
            clear_thread_logger(logger);
            close(*clisock);
            pthread_exit(0);
        }
    }

    return buf;
}

// signal handling 

void sig_handler(int signo)
{
    thread_logger *thl_sig = new_thread_logger(debug_mode);

    if (signo == SIGINT) // control+c
    {
        printf("\nInterrupt request detected, exiting...\n");
        int delfile = system("rm -rf /dev/shm/linkup-varstore/thread_count");
        if (delfile <= -1) { 
            LOGF_ERROR(thl_sig, 0, "Failed to remove file, %s (Error code %d): ", strerror(errno), errno); 
        }

        int deldir = remove("/dev/shm/linkup-varstore/");
        if (deldir == -1) { 
            LOGF_ERROR(thl_sig, 0, "Failed to remove dir, %s (Error code %d): ", strerror(errno), errno); 
        }
        clear_thread_logger(thl_sig);
        exit(0);
    }

    if (signo == SIGSEGV) // segfault
    {
        LOGF_ERROR(thl_sig, 0, "Segfault caught, exiting...", "printf");
        int delfile = system("rm -rf /dev/shm/linkup-varstore/thread_count");
        if (delfile <= -1) { 
            LOGF_ERROR(thl_sig, 0, "Failed to remove file, %s (Error code %d): ", strerror(errno), errno); 
        }

        int deldir = remove("/dev/shm/linkup-varstore/");
        if (deldir == -1) { 
            LOGF_ERROR(thl_sig, 0, "Failed to remove dir, %s (Error code %d): ", strerror(errno), errno); 
        }
        clear_thread_logger(thl_sig);
        exit(0);
    }
}

// basic utils

int showhelp()
{
    printf("\n");
    printf("clearscr : clears screen\n");
    printf("showthreads : shows current amount of running threads\n");
    printf("showbans : shows current ban list\n");
    printf("ban <ip address> : prevents specified IP address from opening connection threads to the server\n");
    printf("unban <ip address>: unbans banned IP address\n");
    printf("help/cmds : shows help/cmds\n");
    printf("exit : gracefully exits the program\n");
    printf("\n");

    return 0;
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
    thread_logger *thl_recverr = new_thread_logger(debug_mode);
    LOGF_ERROR(thl_recverr, 0, "Error reading from socket : (TID : %ld)", TID);
    LOGF_ERROR(thl_recverr, 0, "%s (Error code %d)", strerror(errno), errno);
    
    errno = 0; // reset global errno
    clear_thread_logger(thl_recverr);
}

void print_send_err(int TID)
{
    thread_logger *thl_senderr = new_thread_logger(debug_mode);
    LOGF_ERROR(thl_senderr, 0, "Error writing to socket : (TID : %ld\n", TID);
    LOGF_ERROR(thl_senderr, 0, "%s (Error code %d) \n", strerror(errno), errno)
    
    errno = 0;
    clear_thread_logger(thl_senderr);
}

void thread_store(enum THREAD_STORE_OPTION opt)
{
    FILE *fptr;
    thread_logger *thl_thread_store = new_thread_logger(debug_mode);

    if (opt == create)
    {
        LOGF_DEBUG(thl_thread_store, 0, "Creating directory for shared memory storage", "printf");
        if (system("mkdir /dev/shm/linkup-varstore") <= -1)
        {
            printf("possible system() error : %s (Error code %d)\n", strerror(errno), errno);
            errno = 0; // keeping errno fresh incase current function didn't call it
        }
    }

    if (opt == update)
    {
        int delfile = system("rm -rf /dev/shm/linkup-varstore/thread_count");
        if (delfile <= -1) { LOGF_ERROR(thl_thread_store, 0, "Failed to remove file, %s (Error code %d): ", strerror(errno), errno); }

        int deldir = remove("/dev/shm/linkup-varstore/");
        if (deldir == -1) { LOGF_ERROR(thl_thread_store, 0, "Failed to remove dir, %s (Error code %d): ", strerror(errno), errno); }

        int sysmkdir = system("mkdir /dev/shm/linkup-varstore/");
        if (sysmkdir <= -1) { LOGF_ERROR(thl_thread_store, 0, "possible system() mkdir error : %s (Error code %d)\n", strerror(errno), errno); }

        int sysappend = system(appendcmd);
        if (sysappend <= -1) { LOGF_ERROR(thl_thread_store, 0, "possible syste() appendcmd error : %s (Error code %d)\n", strerror(errno), errno); }

        fptr = fopen("/dev/shm/linkup-varstore/thread_count", "r"); //fopen automatically creates this file for us if it doesn't exist, if entire directory does not exist however, we need to manually create the dir (which is why we call mkdir before)
        while (!feof(fptr))                                         // segfault if no file or directory
        {
            if (fscanf(fptr, "%d", &thread_count) == 0) {
                printf("possible fscaf error : %s (Error code %d)\n", strerror(errno), errno);
                errno = 0;
            }
        }

        thread_count = thread_count - timer_thread_count - 2; // minus two because we're not including cmd_input thread and main thread, 
                                                              // minus timer_thread_count because not including timer threads
        fclose(fptr);
        remove("/dev/shm/linkup-varstore/");
    }
    clear_thread_logger(thl_thread_store);
}

int update_used_ipaddr_elements() 
{
    int cnt = 0;

    for (int i=0; i<=banned_rawlen; i++) 
    {
        if (strcmp(banned_addresses[i], "") != 0)  { // if is not blank
            // printf("VALID IP : %s\n", banned_addresses[i]);
            cnt++;
        }
    }
    cnt = cnt - 1; // to start at zero, just as the actual array does
    return cnt;
}

void banlist_append_ipaddr(char *THREAD_IP)
{
    for (int i=0; i<=ipsignal_rawlen; i++)
    {
        if(strcmp(ipsignal.SIGNAL_IP[i], THREAD_IP) == 0)
        {
            // clearing signal IP incase stuck
            memset(ipsignal.SIGNAL_IP[i], '\0', sizeof(ipsignal.SIGNAL_IP[i]));
        }
    }

    bool already_stored = false;

    // to prevent making duplicates
    for (int i = 0; i <= banned_rawlen; i++)
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
        for (int i = 0; i <= timer_rawlen; i++)
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
    for (int i=0; i<=ipsignal_rawlen; i++)
    {
        if(strcmp(ipsignal.SIGNAL_IP[i], THREAD_IP) == 0)
        {
            // clearing signal IP incase stuck
            memset(ipsignal.SIGNAL_IP[i], '\0', sizeof(ipsignal.SIGNAL_IP[i]));
        }
    }

    thread_logger *thl_banlist_remove_ipaddr = new_thread_logger(debug_mode);
    for (int i = 0; i <= banned_rawlen; i++)
    {
        if (strcmp(banned_addresses[i], THREAD_IP) == 0)
        {
            LOGF_DEBUG(thl_banlist_remove_ipaddr, 0 ,"IP address %s removed from banlist\n", THREAD_IP, "printf");
            strncpy(banned_addresses[i], "", 1);
            break;
        }

        else if (strcmp(banned_addresses[i], THREAD_IP) != 0)
        { // if doesn't match input IP address
            continue;
        }
    }

    clear_thread_logger(thl_banlist_remove_ipaddr);
}

void timer_signal_ran(char *THREAD_IP, thread_logger *logger) // tells timer thread that the client is talking to us and has ran something
{
    bool submitted = false;

    for (int i=0; i<=ipsignal_rawlen; i++)
    {
        if (strcmp(ipsignal.SIGNAL_IP[i],"") == 0) // if blank, let's populate this one
        {
            strncpy(ipsignal.SIGNAL_IP[i], THREAD_IP, sizeof(ipsignal.SIGNAL_IP[i]));
            usleep(260 * 1000); // 260ms to wait for timer thread to register that we just ran
            strncpy(ipsignal.SIGNAL_IP[i], "", sizeof(ipsignal.SIGNAL_IP[i]));

            submitted = true;
            break;
        }
    }

    if (submitted == false) { LOGF_ERROR(logger, 0, "Could not find an empty array element in ipsignal to popluate, fixme.", "printf"); }
}

void timer_append_ipaddr(char *THREAD_IP)
{
    bool already_stored = false;

    // to prevent making duplicates
    for (int i = 0; i <= timer_rawlen; i++)
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
        for (int i = 0; i <= timer_rawlen; i++)
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
    for (int i = 0; i <= timer_rawlen; i++)
    {
        if (strcmp(timed_addresses[i], THREAD_IP) == 0)
        {
            printf("Removing %s from timer list\n", timed_addresses[i]);
            strncpy(timed_addresses[i], "", 1);
            break;
        }

        else if (strcmp(timed_addresses[i], THREAD_IP) != 0)
        { // if doesn't match input IP address
            continue;
        }
    }
}