#ifndef SESSIONINFO_H
#define SESSIONINFO_H
    #define MAX_SESSION_STRUCTS 60 // this should never be a higher value than MAX_GLOBAL_THREADS
    #define NO_ID 24706 // this is basically a unique null for when we have to pass the ID but don't want to
    #define NO_CONNECTION_TID 38671
    #define NO_STATUS 86284

    typedef enum {
        READY,
        BUSY
    } state;

    struct SessionInfoNode {
        char THREAD_IP[16];
        char HWID[19];
        int CONNECTION_TID;

        char CONNECT_CODE[9];
        state STATUS;
        int ID;

        struct SessionInfoNode* NEXT;
        struct SessionInfoNode* PREV;
    };
    typedef struct SessionInfoNode SessionInfoNode_T;

    void add_node(SessionInfoNode_T** head_ref, char *THREAD_IP, char *HWID, int CONNECTION_TID, char *CONNECT_CODE, state STATUS);
    SessionInfoNode_T* find_node(SessionInfoNode_T* head, char *CONNECT_CODE, char *THREAD_IP, int ID);
    void delete_node(SessionInfoNode_T** head_ref, int ID);
    void print_list(SessionInfoNode_T* head);
#endif