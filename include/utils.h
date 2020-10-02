#ifndef UTILS_H
#define UTILS_H

// ===========utils.c function/structs============== //

int update_used_ipaddr_elements();
int showhelp();
int usleep(__useconds_t usec); // microsecond sleep

void print_recv_err(int TID);
void print_send_err(int TID);

void timer_append_ipaddr(char *THREAD_IP);
void timer_remove_ipaddr(char *THREAD_IP);

void banlist_append_ipaddr(char *THREAD_IP);
void banlist_remove_ipaddr(char *THREAD_IP);

void thread_store(enum THREAD_STORE_OPTION opt);
void sig_handler(int signo);

// char info functions
void priter(char* tempstring);
int lengthofchar(char* tempstring);

// ===========timer.c function/structs============== //

struct timerblock {
    float seconds_passed;
    int times_ran;
};

struct connected {
    char SIGNAL_IP[4][16];
};
struct connected ipsignal;

void handle_timer(void *VPTR_THREAD_IP);

// =============mysql.c function/structs=============== //

int mysql_main();

#endif