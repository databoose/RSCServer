#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mysql/mysql.h>

#include <stdarg.h>

#include "include/logger.h"
#include "include/main.h"

static char *host = "localhost";
static char *user = "linkup";
static char *pass = "linkup-1337";
static char *dbname = "linkup_db";

unsigned short int port = 3306; // 3306 is default mysql port
static char *unix_socket = NULL;
unsigned int flag = 0;

char query[85]; // below 80 seems to cause buffer overflow from sprintf, beware

void insert_query(MYSQL *conn, char* table, char* column1, char* column2, char* value1, char* value2)
{
    memset(query, '\0', sizeof(query));
    sprintf(query, "INSERT INTO %s(%s,%s) values('%s','%s');", table, column1,column2, value1,value2);
    // printf("Submitting query : %s\n", query);

    if(mysql_query(conn, query)) {
        printf("MySQL query error : %s\n",mysql_error(conn)); // Returns the error message for the most recently invoked MySQL function. 
    }
    
    else {
        printf("Successfully inserted\n");
    }
}

void print_table_contents(MYSQL *conn, char* table)
{
    memset(query, '\0', sizeof(query)); // zeroing out query string incase some remains
    sprintf(query, "SELECT * FROM %s", table); // select all fields in table that we specify in the print_table_contents call
    if(mysql_query(conn, query)) {
        mysql_error(conn);
    }
    MYSQL_RES *result = mysql_store_result(conn); // we declare and define result after we do the query
    MYSQL_ROW row; // MYSQL_ROW represents an array of character pointers, pointing to the columns of the actual data row

    int field_count = mysql_num_fields(result); // Returns the number of columns in a result set.
    while ((row = mysql_fetch_row(result)))
    {
        for(int i = 0; i < field_count; i++)
        {
           printf("[%d] %s\n", i, row[i]);
        }
    }
    mysql_free_result(result);
}

int mysql_register(char *ipaddr, char *hwidhash)
{
    MYSQL *conn;
    conn = mysql_init(NULL); //initalizes MYSQL structs

    // try to connect, if fails print error
    if(!(mysql_real_connect(conn, host, user, pass, dbname, port, unix_socket, flag))) {
        printf("MySQL query error : %s\n",mysql_error(conn));
        LOGF_ERROR(global_thl, 0 ,"Refusing to run mysql routine, mysql server most likely not running or something else horribly wrong.", "printf");
        mysql_close(conn);
        return 0;
    }

    if(conn->thread_id == 0)
    {
        LOGF_ERROR(global_thl, 0 ,"Refusing to run mysql routine, mysql server most likely not running or something else horribly wrong. (thread_id == 0)", "printf");
        mysql_close(conn);

        return 0;
    }

    if(conn->thread_id > 0) // if a mysql thread actually exists and we have a session
    {
        insert_query(conn, "users", "hwidhash", "ipaddr", 
                                     hwidhash, ipaddr);
        mysql_close(conn);
        return 1;
    }

    return 1;
}