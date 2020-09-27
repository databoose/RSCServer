#ifndef UTILS_H
#define UTILS_H

// ===========utils.c function/structs============== //

int update_used_ipaddr_elements();
int showhelp();
int usleep(__useconds_t usec); // microsecond sleep

void append_ipaddrtimer(char *THREAD_IP);
void remove_ipaddrtimer(char *THREAD_IP);
void append_ipaddrban(char *THREAD_IP);
void remove_ipaddrban(char *THREAD_IP);
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