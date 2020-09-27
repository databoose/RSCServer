#ifndef MAIN_H
#define MAIN_H

#define TIMEOUT_IN 25 // seconds
#define TIMEOUT_OUT 25

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
char banned_addresses[18][16];
char timed_addresses[50][16];

// these externs are defined and changed as needed in main.c (primarily)
extern bool banning;
extern bool unbanning;
extern bool debug_mode;

// cmd_input.c
void cmd_input();

#endif