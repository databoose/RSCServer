#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mysql/mysql.h>

static char *host = "localhost";
static char *user = "linkup";
static char *pass = "linkup-1337";
static char *dbname = "linkup_db";

unsigned short int port = 3306; // 3306 is default mysql port
static char *unix_socket = NULL;
unsigned int flag = 0;

void insert_query(MYSQL *conn, char* query)
{
    if(mysql_query(conn, query)) {
        printf("MySQL query error : %s\n",mysql_error(conn)); // Returns the error message for the most recently invoked MySQL function. 
    }
    
    else {
        printf("Successfully inserted\n\n");
    }
}

void print_table_contents(MYSQL *conn, char* table)
{
    char query[50];

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

void mysql_main()
{
    MYSQL *conn;
    conn = mysql_init(NULL); //initalizes MYSQL structs

    // try to connect, if fails print error
    if(!(mysql_real_connect(conn, host, user, pass, dbname, port, unix_socket, flag))) {
        fprintf(stderr, "\nError: %s [%d]\n", mysql_error(conn), mysql_errno(conn));
        exit(1);
    }
    else {
        printf("Connection successful\n\n");
    }
    insert_query(conn,"INSERT INTO user(hwidhash_uid,ip_address) values('hashhere','iphere2');");
    print_table_contents(conn,"user");
    
    mysql_close(conn);
}