rm debug-exec
cd /home/data/Desktop/c_projects/LinkUpServer
gcc -g -O0 -std=c99 main.c colors.c logger.c mysql.c cmd_input.c utils.c `mysql_config --cflags --libs` -lpthread -o debug_builds/debug-exec