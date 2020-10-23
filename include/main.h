#ifndef MAIN_H
#define MAIN_H

#define TIMEOUT_IN 25 // seconds
#define TIMEOUT_OUT 25

#define BANNED_RAWLEN_SIZE 18
#define TIMER_RAWLEN_SIZE 35
#define IPSIGNAL_RAWLEN_SIZE 5

enum THREAD_STORE_OPTION {
    create,
    update
};

enum MAIN_OPTION {
    skip_to_connloop
};

// main.c stuff
int thread_count;
int timer_thread_count;

int banned_rawlen;
int timer_rawlen;
int ipsignal_rawlen;

int iparry_usedamount;
int servsockfd; // global so we can pass it around functions

pid_t self_pid;
char appendcmd[64];

// WARNING: ALSO CHANGE BANNED/TIMER/IPSIGNAL_RAWLEN_SIZE MACROS IF YOU CHANGE THE SIZE OF THESE
char signal_addresses[5][16]; // stores up to 5 IP addresses at one time
char banned_addresses[18][16];
char timed_addresses[35][16];

// these externs are defined and changed as needed in main.c (primarily)
extern bool banning;
extern bool unbanning;
extern bool debug_mode;

// cmd_input.c
void cmd_input();

#endif