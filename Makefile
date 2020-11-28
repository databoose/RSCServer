OBJS	= main.o mysql.o colors.o logger.o utils.o cmd_input.o timer.o session_info.o
SOURCE	= main.c mysql.c colors.c logger.c utils.c cmd_input.c timer.c session_info.c
HEADER	=
OUT	= RSC_Server
CC	 = ccache gcc # for compiler caching

FLAGS	 =  -c -O2 -Wall -Wmain -std=c99 -fdata-sections -ffunction-sections # last two options remove dead code
LFLAGS	 = `mysql_config --cflags --libs` -lpthread -pthread

all: $(OBJS)
	$(CC) $(OBJS) -o $(OUT) $(LFLAGS)

main.o: main.c
	$(CC) $(FLAGS) main.c

session_info.o: session_info.c
	$(CC) $(FLAGS) session_info.c

mysql.o: mysql.c
	$(CC) $(FLAGS) mysql.c

colors.o: colors.c
	$(CC) $(FLAGS) colors.c

logger.o: logger.c
	$(CC) $(FLAGS) logger.c

utils.o: utils.c
	$(CC) $(FLAGS) utils.c

cmd_input.o: cmd_input.c
	$(CC) $(FLAGS) cmd_input.c

timer.o: timer.c
	$(CC) $(FLAGS) timer.c

clean:
	rm -f $(OUT) $(OBJS)