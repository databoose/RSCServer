#ifndef UTILS_H
#define UTILS_H

// ===========utils.c function/structs============== //

void safesend(int * clisock, int TID, thread_logger *logger, char *buf);
char *saferecv(int * clisock, int TID, thread_logger *logger, size_t len, char* type, char *expected_string);

int usleep(__useconds_t usec); // microsecond sleep

void print_recv_err(int TID);
void print_send_err(int TID);

void timer_append_ipaddr(char *THREAD_IP);
void timer_remove_ipaddr(char *THREAD_IP);
void timer_signal_ran(char *THREAD_IP);

void banlist_append_ipaddr(char *THREAD_IP);
void banlist_remove_ipaddr(char *THREAD_IP);

void thread_store(enum THREAD_STORE_OPTION opt);
void sig_handler(int signo);

// char functions
char *strremove(char *str, const char *sub);
void priter(char* tempstring);
int lengthofstring(char* tempstring);

// ===========timer.c function/structs============== //

struct timerblock {
    float seconds_passed;
    int times_ran;
};

void handle_timer(void *VPTR_THREAD_IP);

// =============mysql.c function/structs=============== //

int mysql_register(char *ipaddr, char *hwidhash);

#endif