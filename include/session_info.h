#ifndef SESSIONINFO_H
#define SESSIONINFO_H
    #define MAX_SESSION_STRUCTS 60 // this should never be a higher value than MAX_GLOBAL_THREADS
    #define NO_ID 24706 // this is basically a unique null for when we have to pass the ID but don't want to
    #define NO_CONNECTION_TID 38671
    #define NO_STATUS 86284

    #define THREAD_IP_SIZE 17 // 1 extra byte for null termination that will be inserted with strncpy()
    #define HWID_SIZE 21
    #define CONNECT_CODE_SIZE 12

    typedef enum {
        READY,
        BUSY
    } state;

    struct SessionInfoNode {
        char THREAD_IP[THREAD_IP_SIZE];
        char HWID[HWID_SIZE];
        int CONNECTION_TID;

        char CONNECT_CODE[CONNECT_CODE_SIZE];
        state STATUS;
        int ID;

        struct SessionInfoNode* NEXT;
        struct SessionInfoNode* PREV;
    };
    typedef struct SessionInfoNode SessionInfoNode_T;
    extern SessionInfoNode_T* LIST_HEAD;

    void add_node(SessionInfoNode_T** head_ref, char *THREAD_IP, char *HWID, int CONNECTION_TID, char *CONNECT_CODE, state STATUS);
    SessionInfoNode_T* find_node(SessionInfoNode_T* head, char *CONNECT_CODE, char *THREAD_IP, char *HWID, int ID);
    void delete_node(SessionInfoNode_T** head_ref, int ID);
    void print_list(SessionInfoNode_T* head);
#endif