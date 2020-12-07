#!/bin/bash

rm debug-exec
cd .. # nativgate up a directory
printf "Compiling ...\n"
gcc -g -O0 -std=c99 main.c colors.c logger.c mysql.c cmd_input.c utils.c timer.c session_info.c `mysql_config --cflags --libs` -lpthread -o debug_builds/debug-exec

printf "Compiled\n"
