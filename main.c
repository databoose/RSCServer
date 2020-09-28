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
struct connected ipsignal;

/*
    WATCH OUT FOR :
            1. using strncpy with same exact buffer size of destination string, may cut off null teminator
            2. using only 1 as buffer length for "" when emptying a string
            
            3. improper breaks in nested if statements and or loops
*/

/*
    TODO: 
          1. implement mysql shit maybe?
          
          2. deal with insert_query in mysql.c
          3. deal with print_table_contents in mysql.c
          
          (somewhat kinda works) make timing out much faster and responsive, currently it only seems to timeout an IP after the flood of connections

          ( SOMEWHAT FIXED ) somehow optimize do while loop in timer thread to not spam cpu as hard
          ( FIXED ? ) prevent currrent timer thread from dropping when it's not supposed to
          ( FIXED ? ) fix spam banning, make it happen only once when activated
          ( FIXED ) do proper strln implementatin to prevent buffer overflow
          ( FIXED) look out for banning fucking up the next element
          ( IMPLEMENTED ) eventually cut off the timer thread after the client doesn't talk for a while (aka timeout)
*/

void set_timeout(int servsockfd, int timeout_input_seconds, int timeout_output_seconds)
{
    /*
        Setting options :
        - SO_RCVTIMO, to set timeout for input operations
        - SO_SNDTIMEO, to set timeout for output operations
    */

    struct timeval timeout;
    timeout.tv_usec = 0;
    timeout.tv_sec = 0;

    timeout.tv_sec = timeout_input_seconds;
    if (setsockopt(servsockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0)
    {
        printf("setsockopt so_rcvtimeo unsuccessful : %s (Error code %d)\n", strerror(errno), errno);
        errno = 0;
    }

    else
    {
        timeout.tv_sec = timeout_output_seconds;
        if (setsockopt(servsockfd, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)) < 0)
        {
            printf("setsockopt so_sndtimo unsuccessful : %s (Error code %d)\n", strerror(errno), errno);
            errno = 0;
        }
    }
    //printf("Timeout set\n");
}

void handle_connection(void *p_clisock) // thread functions need to be a void pointer, args can be void pointer or directly referenced via & pointer when working with ints
{
    int TID = rand() % (999999999 + 1 - 100000000) + 100000000; // (max_number + 1 - minimum_number) + minimum_number
    thread_logger *thl = new_thread_logger(debug_mode);
    char *THREAD_IP = inet_ntoa(cli_addr.sin_addr);

    bool submitted = false;

    for (int i=0; i<=ipsignal_rawlen; i++)
    {
        if (strcmp(ipsignal.SIGNAL_IP[i],"") == 0) // if blank, let's populate this one
        {
            strncpy(ipsignal.SIGNAL_IP[i], THREAD_IP, sizeof(ipsignal.SIGNAL_IP[i]));
            usleep(380 * 1000); // 380ms to wait for timer thread to register that we just ran
            strncpy(ipsignal.SIGNAL_IP[i], "", sizeof(ipsignal.SIGNAL_IP[i]));

            submitted = true;
            break;
        }
    }

    if (submitted == false) { LOGF_ERROR(thl, 0, "Could not find an empty array element in ipsignal to popluate, fixme.", "printf"); }

    int clisock = *((int *)p_clisock); // dereference pointer
    free(p_clisock);                   // we don't need the pointer anymore.

    // at this point, do whatever you want to here, the code below is specific to this application

    char *verif_recv_str = "Ar4#8Pzw<&M00Nk";
    char verif_send_str[110] = "4Ex{Y**y8wOh!T00";
    char recv_buf[110];

    int recv_status = recv(clisock, (void *)recv_buf, (size_t)sizeof(recv_buf), 0);
    if (recv_status == -1)
    {
        print_recv_err(TID);
        close(clisock);
        pthread_exit(0);
    }

    LOGF_DEBUG(thl, 0, "Waiting for verification string from client ... ", "printf");
    if (strcmp(verif_recv_str, recv_buf) == 0)
    {
        LOGF_DEBUG(thl, 0, "Verified", "printf");
        memset(recv_buf, '\0', sizeof(recv_buf));

        int send_status = send(clisock, (void *)verif_send_str, (size_t)lengthofchar(verif_send_str), 0);
        if (send_status == -1)
        {
            print_send_err(TID);
            close(clisock);
            pthread_exit(0);
        }
    }

    else
    {
        LOGF_ERROR(thl, 0, "Not verified, verification string does not match from server to client...", "printf");
        LOGF_DEBUG(thl, 0 , "Exiting thread due to verification failure", "printf");
        clear_thread_logger(thl);
        close(clisock);
        pthread_exit(0);
    }

    // done with verification, now we wanna get user's hwid hash

    recv_status = recv(clisock, (void *)recv_buf, (size_t)sizeof(recv_buf), 0);
    if (recv_status == -1)
    {
        print_recv_err(TID);
        close(clisock);
        pthread_exit(0);
    }

    mysql_main();

    // done with whatever we want to do, now quit
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
        
        memset(banned_addresses, '\0', sizeof(banned_addresses));
        memset(&cli_addr, 0, sizeof(struct sockaddr_in)); //initializing cli_addr struct

        banned_rawlen = sizeof(banned_addresses) / sizeof(banned_addresses[0]);
        ipsignal_rawlen = sizeof(ipsignal.SIGNAL_IP) / sizeof(ipsignal.SIGNAL_IP[0]);
        timer_rawlen = sizeof(timed_addresses) / sizeof(timed_addresses[0]);
        
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
