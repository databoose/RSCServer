#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>

#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdbool.h>
#include <dirent.h>

#define PORT 64912
#define SA struct sockaddr // cast shortener

#define MAX_SERVER_BACKLOG 25 // max 25 clients in lobby
#define MAX_GLOBAL_THREADS 60 // max 60 clients
#define MAX_IPUSER_THREADS 5

#include "include/logger.h"
#include "include/main.h"
#include "include/utils.h"
#include "include/session_info.h"

bool debug_mode = true; // we also want debug logs, only disable this when doing live deployment
bool banning = false;
bool unbanning = false;

struct sockaddr_in serv_addr;
struct sockaddr_in cli_addr;

static pthread_mutex_t rand_mutex = PTHREAD_MUTEX_INITIALIZER;

void set_timeout(int servsockfd, int timeout_input_seconds, int timeout_output_seconds)
{
    /*
        Setting options :
        - SO_RCVTIMO, to set timeout for input operations
        - SO_SNDTIMEO, to set timeout for output operations
    */

    thread_logger* thl_set_timeout = new_thread_logger(debug_mode);

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
            LOGF_DEBUG(thl_set_timeout, 0, "setsockopt so_sndtimo unsuccessful : %s (Error code %d)\n", strerror(errno), errno, "printf");
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
    int *clisock_ptr = &clisock;
    thread_logger *thl = new_thread_logger(debug_mode);

    pthread_mutex_lock(&rand_mutex); // setting lock everytime we pull from /dev/urandom to prevent other users pulling same values
    int CONNECTION_TID;
    FILE *urand_ptr;
    urand_ptr = fopen("/dev/urandom", "rb");
    if (fread(&CONNECTION_TID, 1, sizeof(int), urand_ptr) <= 0) {
        LOGF_ERROR(thl, 0, "fread error, returned 0 or below", "printf");
        LOGF_ERROR(thl, 0, "%s", strerror(errno), "printf");
    }
    if (CONNECTION_TID < 0) {
        CONNECTION_TID = abs(CONNECTION_TID);
    }
    fclose(urand_ptr);
    pthread_mutex_unlock(&rand_mutex);

    LOGF_DEBUG(thl, 0, "CONNECTION TID : %d", CONNECTION_TID, "printf");
    //LOGF_DEBUG(thl,0, "Mutex unlocked (TID : %d)", CONNECTION_TID, "printf");

    char *THREAD_IP = inet_ntoa(cli_addr.sin_addr);
    timer_signal_ran(THREAD_IP, thl); // run this everytime a user action ran

    // at this point, do whatever you want to here, the code below is specific to this application

    char *ret_ptr = saferecv(clisock_ptr, CONNECTION_TID, thl, lengthofstring("Ar4#8Pzw<&M00Nk"), "Ar4#8Pzw<&M00Nk");
    free(ret_ptr);                                                    // frees malloced return value from saferecv
    safesend(clisock_ptr, CONNECTION_TID, thl, "4Ex{Y**y8wOh!T00\n"); // telling client we got its verification response string

    char *hwid_string = saferecv(clisock_ptr, CONNECTION_TID, thl, 20, NULL); // 16 bytes for hwid hash, + 4 bytes for prefix "ny3_"
    if (strstr(hwid_string, "ny3_") != NULL)
    {
        // LOGF_DEBUG(thl, 0, "Client HWID : %s\n", hwid_string, "printf");
    }
    else
    {
        LOGF_ERROR(thl, 0, "Expected HWID but missing HWID prefix, string is : %s", hwid_string, "printf");
        LOGF_ERROR(thl, 0, "Closing thread %d\n", CONNECTION_TID, "printf");

        clear_thread_logger(thl);
        close(clisock);
        pthread_exit(0);
    }

    // verification finished, now wait for client to tell us it is at lobby

    mysql_register(THREAD_IP, hwid_string);
    free(hwid_string);
    char *retc = saferecv(clisock_ptr, CONNECTION_TID, thl, lengthofstring("inlobby"), "inlobby");
    free(retc);

    pthread_mutex_lock(&rand_mutex);
    int malint;
    urand_ptr = fopen("/dev/urandom", "rb");
    if (fread(&malint, 1, sizeof(int), urand_ptr) <= 0) {
        LOGF_ERROR(thl, 0, "fread error, returned 0 or below", "printf");
        LOGF_ERROR(thl, 0, "%s", strerror(errno), "printf");
    }
    if (malint < 0) {
        malint = abs(malint);
    }
    fclose(urand_ptr);
    pthread_mutex_unlock(&rand_mutex);

    char *malstr = malloc(11 + 2);
    snprintf(malstr, 8, "%d", malint); // put only 8 bytes of malint into malstr (7 numbers)
    strcat(malstr, "\n"); // add newline to end (so we can actually send this to client)

    safesend(clisock_ptr, CONNECTION_TID, thl, malstr);
    free(malstr);

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
        NULLSTRING = "Z&fw&pok5o!itKU!s";
        if (signal(SIGINT, sig_handler) == SIG_ERR) {
            LOGF_ERROR(thl, 0, "\ncan't catch SIG", "printf");
        }

        thread_store(create);
        sprintf(appendcmd, "ps hH p %d | wc -l > /dev/shm/linkup-varstore/thread_count", getpid());

        memset(&cli_addr, 0, sizeof(struct sockaddr_in)); //initializing cli_addr struct
        servsockfd = socket(AF_INET, SOCK_STREAM, 0);
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(PORT);
        serv_addr.sin_addr.s_addr = inet_addr("10.0.0.225");

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
            if (errno == 99) {
                LOGF_ERROR(thl, 0, "Wrong IP address, or system not connected to router perhaps?", NULL);
                LOGF_ERROR(thl, 0, "Check if the configured IP in main.c is incorrect, IP could have changed...",NULL);
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
        if (clisock >= 0)                                              // if client connects
        {
            for (int i = 0; i <= BANNED_RAWLEN_SIZE - 1; i++) // BANNED_RAWLEN_SIZE - 1 because we want to start at element 0 through the banned_addresses arrays
            {
                if (strcmp(inet_ntoa(cli_addr.sin_addr), banned_addresses[i]) == 0)
                {
                    LOGF_WARN(thl, 0, "Banned IP address %s attempted to connect, refusing to open connection thread", inet_ntoa(cli_addr.sin_addr));
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
