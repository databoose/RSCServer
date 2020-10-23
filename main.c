#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdbool.h>
#include <dirent.h>

#include <sys/time.h>

#define PORT 64912

#define SA struct sockaddr  // cast shortener

#define MAX_SERVER_BACKLOG 25   // max 25 clients in lobby
#define MAX_GLOBAL_THREADS 60 // max 60 clients
#define MAX_IPUSER_THREADS 5

#include "include/logger.h"
#include "include/main.h"
#include "include/utils.h"

bool debug_mode = true;
bool banning = false;
bool unbanning = false;

struct sockaddr_in serv_addr;
struct sockaddr_in cli_addr;

/*
    WATCH OUT FOR :
            -  accessing arrays out of bound
            -  using strncpy with same exact buffer size of destination string, may cut off null teminator
            -  using only 1 as buffer length for "" when emptying a string
            -  improper breaks in nested if statements and or loops

            - please jesus for the love of god always add a fuckign newline to the end of whatever you send you fucking moron
            you spent 5 hours trying to figure out why a readline() call in java was hanging when you never put a newline to the end
            
            - make sure to keep all of the memory inside mysql.c local
*/

/*
    TODO: 
          1. Deal with mysql implementation, valgrind is complaining about possible leaks related to mysql..
          
          (FIXED, it was because i never added a newline to my outgoing buffer to the client, so the client never knew when the line stopped, so it hung forever until socket closure) CRITICAL : CLIENT ONLY SEEMS TO GET SEND() FROM SERVER IF SOCKET CLOSES, FIX
          ( FIXED, TURNED OUT TO BE OUT OF BOUNDS MEMORY BUG ) Fix bug where timer thread constantly increments block.times_ran after a connection and a ban.
          ( FIXED, TURNED OUT TO BE CLIENT ) make timing out much faster and responsive, currently it only seems to timeout an IP after the flood of connections
          ( SOMEWHAT FIXED ) somehow optimize do while loop in timer thread to not spam cpu as hard
          ( FIXED ? ) prevent currrent timer thread from dropping when it's not supposed to
          ( FIXED ? ) fix spam banning, make it happen only once when activated
          ( FIXED ) do proper strln implementatin to prevent buffer overflow
          ( FIXED ) look out for banning fucking up the next element
          ( IMPLEMENTED ) eventually cut off the timer thread after the client doesn't talk for a while (aka timeout)
          ( IMPLEMENTED ) Receive the hwid hash from client, once you have implemented it in the client to send the hash.
*/

void set_timeout(int servsockfd, int timeout_input_seconds, int timeout_output_seconds)
{
    /*
        Setting options :
        - SO_RCVTIMO, to set timeout for input operations
        - SO_SNDTIMEO, to set timeout for output operations
    */

    thread_logger *thl_set_timeout = new_thread_logger(debug_mode);

    struct timeval timeout;
    timeout.tv_usec = 0;
    timeout.tv_sec = 0;

    timeout.tv_sec = timeout_input_seconds;
    if (setsockopt(servsockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0)
    {
        LOGF_DEBUG(thl_set_timeout, 0, "setsockopt so_rcvtimeo unsuccessful : %s (Error code %d)\n", strerror(errno), errno, "printf");
        errno = 0;
    }

    else
    {
        timeout.tv_sec = timeout_output_seconds;
        if (setsockopt(servsockfd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)) < 0)
        {
            LOGF_DEBUG(thl_set_timeout, 0 ,"setsockopt so_sndtimo unsuccessful : %s (Error code %d)\n", strerror(errno), errno, "printf");
            errno = 0;
        }
    }
    clear_thread_logger(thl_set_timeout);
    //printf("Timeout set\n");
}

void handle_connection(void *p_clisock) // thread functions need to be a void pointer, args can be void pointer or directly referenced via & pointer when working with ints
{
    int clisock = *((int *)p_clisock); // dereference pointer
    free(p_clisock);                   // we don't need the pointer anymore.
    int * clisock_ptr = &clisock;

    int CONNECTION_TID = rand() % (999999999 + 1 - 100000000) + 100000000; // (max_number + 1 - minimum_number) + minimum_number
    thread_logger *thl = new_thread_logger(debug_mode);
    char *THREAD_IP = inet_ntoa(cli_addr.sin_addr);
    LOGF_DEBUG(thl, 0, "CONNECTION TID : %d", CONNECTION_TID, "printf");

    timer_signal_ran(THREAD_IP,thl); // run this everytime a user action ran

    // at this point, do whatever you want to here, the code below is specific to this application
    
    char *ret_ptr = saferecv(clisock_ptr, CONNECTION_TID, thl, lengthofstring("Ar4#8Pzw<&M00Nk"), "Ar4#8Pzw<&M00Nk");
    free(ret_ptr); // frees malloced return value from saferecv
    safesend(clisock_ptr, CONNECTION_TID, thl, "4Ex{Y**y8wOh!T00\n"); // telling client we got its verification response string
    
    char *hwid_string = saferecv(clisock_ptr, CONNECTION_TID, thl, 20, NULL); // 16 bytes for hwid hash, + 4 bytes for prefix "ny3_"
    
    if(strstr(hwid_string, "ny3_") != NULL) { 
        // LOGF_DEBUG(thl, 0, "Client HWID : %s\n", hwid_string, "printf");
    }
    else {
        LOGF_ERROR(thl, 0, "Expected HWID but missing HWID prefix, string is : %s", hwid_string, "printf");
        LOGF_ERROR(thl, 0, "Closing thread %d\n", CONNECTION_TID, "printf");

        clear_thread_logger(thl);
        close(clisock);
        pthread_exit(0);
    }
    
    mysql_main(THREAD_IP, hwid_string);
    free(hwid_string);

    // done with whatever we want to do, now quit
    LOGF_DEBUG(thl, 0, "Connection thread done , closing connection thread (CONNECTION TID: %d)", CONNECTION_TID, "printf");
    clear_thread_logger(thl);
    close(clisock);
    pthread_exit(0);
}

int main(enum MAIN_OPTION opt)
{
    thread_logger *thl = new_thread_logger(debug_mode);
    
    if (opt != skip_to_connloop)
    {
        // init shit
        if (signal(SIGINT, sig_handler) == SIG_ERR) { LOGF_ERROR(thl, 0, "\ncan't catch SIG", "printf");}
        memset(&cli_addr, 0, sizeof(struct sockaddr_in)); //initializing cli_addr struct

        banned_rawlen = BANNED_RAWLEN_SIZE;
        timer_rawlen = TIMER_RAWLEN_SIZE;
        ipsignal_rawlen = IPSIGNAL_RAWLEN_SIZE;

        thread_store(create);
        self_pid = getpid();
        sprintf(appendcmd, "ps hH p %d | wc -l > /dev/shm/linkup-varstore/thread_count", self_pid);

        servsockfd = socket(AF_INET, SOCK_STREAM, 0);
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(PORT);
        serv_addr.sin_addr.s_addr = inet_addr("10.0.0.224");

        if (setsockopt(servsockfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0)
        {
            LOGF_ERROR(thl, 0, "setsockopt so_reuseaddr unsuccessful : %s (Error code %d)\n", strerror(errno), errno);
            errno = 0;
        }
        LOGF_DEBUG(thl, 0, "Set reuse", NULL);
        LOGF_DEBUG(thl, 0, "Binding...", NULL);

        int bind_status;
        bind_status = bind(servsockfd, (SA *)&serv_addr, sizeof(serv_addr));
        if (bind_status == -1)
        {
            LOGF_ERROR(thl, 0, "Bind unsuccessful : %s (Error code %d)", strerror(errno), errno);
            if (errno == 99)
            {
                LOGF_ERROR(thl, 0, "Wrong IP address, or system not connected to router perhaps?", NULL);
                exit(0);
            }

            if (errno == 98)
            {
                LOGF_ERROR(thl, 0, "More than one process running maybe?", NULL);
                exit(0);
            }
            errno = 0;
        }

        int listen_status;
        LOGF_INFO(thl, 0, "Listening...", NULL);
        listen_status = listen(servsockfd, MAX_SERVER_BACKLOG); // second param is backlog, aka how many connections can wait in one point in time
        if (listen_status == -1)
        {
            LOGF_ERROR(thl, 0, "Listen unsuccessful : %s (Error code %d)\n", strerror(errno), errno);
            errno = 0;
        }

        // user now has input
        pthread_t input_thread;
        pthread_create(&input_thread, NULL, (void *)cmd_input, NULL);
    }

    // end of init shit
    for (;;)
    {
        int clisock;
        socklen_t cli_addr_size = sizeof(cli_addr);

        clisock = accept(servsockfd, (SA *)&cli_addr, &cli_addr_size); // last two parameters should fill an optionable client struct for info
        if (clisock >= 0) // if client connects
        {
            iparry_usedamount = update_used_ipaddr_elements();
            for (int i = 0; i <= iparry_usedamount; i++)
            {
                if (strcmp(inet_ntoa(cli_addr.sin_addr), banned_addresses[i]) == 0)
                {
                    LOGF_DEBUG(thl, 0, "Banned IP address %s attempted to connect, refusing to open connection thread", inet_ntoa(cli_addr.sin_addr));
                    close(clisock);
                    clear_thread_logger(thl);
                    main(skip_to_connloop); // we escape this forloop and the code below doesn't run
                }

                else if (strcmp(inet_ntoa(cli_addr.sin_addr), banned_addresses[i]) != 0)
                {
                    continue;
                }
            }
            
            thread_store(update);
            if (thread_count >= MAX_GLOBAL_THREADS) 
            {
                LOGF_DEBUG(thl, 0, "MAX_GLOBAL_THREADS cap hit, blocking further connections until a thread is open", "printf");
                close(clisock);
                clear_thread_logger(thl);
                main(skip_to_connloop); // we escape this forloop and the code below doesn't run
            }

            char *THREAD_IP = inet_ntoa(cli_addr.sin_addr);

            pthread_t timer_thread;
            pthread_create(&timer_thread, NULL, (void *)handle_timer, (void *)THREAD_IP);
            set_timeout(servsockfd, TIMEOUT_IN, TIMEOUT_OUT);

            printf("\n");
            LOGF_INFO(thl, 0, "Connection accepted from : %s:%d", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));

            pthread_t sckthread;
            int *pclient = malloc(sizeof(int));
            *pclient = clisock;
            pthread_create(&sckthread, NULL, (void *)handle_connection, pclient);
        }
    }
}
