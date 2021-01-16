#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h> // for pid_t and shit

#include "include/main.h"
#include "include/session_info.h"
#include "include/logger.h"

int node_index = 0;

int add_node(SessionInfoNode_T** head_ref, char *CONNECT_CODE, char *THREAD_IP, char *HWID, int CONNECTION_TID, state STATUS) {
    node_index++;
    if (node_index > MAX_SESSION_STRUCTS) {
        LOGF_ERROR(global_thl, 0, "Cannot add node, max amount of session structs have been allocated", "printf");
        return 0;
    }
    SessionInfoNode_T* new_node = (SessionInfoNode_T*) malloc(sizeof(SessionInfoNode_T));
    // direct string insertions are very unreliable, only use strncpy() function
    if (strcmp(THREAD_IP, NULLSTRING) != 0) {
         strncpy(new_node->THREAD_IP, THREAD_IP, THREAD_IP_SIZE-1);
    }
    if (strcmp(HWID, NULLSTRING) != 0) {
        strncpy(new_node->HWID, HWID, HWID_SIZE-1);
    }
    if (CONNECTION_TID != NO_CONNECTION_TID) {
        new_node->CONNECTION_TID = CONNECTION_TID;
    }
    if (strcmp(CONNECT_CODE, NULLSTRING) != 0) {
        strncpy(new_node->CONNECT_CODE, CONNECT_CODE, CONNECT_CODE_SIZE-1);
        // gets rid of newline that strncpy() adds in for some fucking reason
        new_node->CONNECT_CODE[strcspn(new_node->CONNECT_CODE, "\n")] = 0;
    }
    if (STATUS != NO_STATUS) {
        new_node->STATUS = STATUS;
    }
    new_node->ID = node_index;

    // make next of new node as head and previous as NULL
    new_node->NEXT = (*head_ref);
    new_node->PREV = NULL;

    // change PREV of head node to new node
    if ((*head_ref) != NULL) 
        (*head_ref)->PREV = new_node;
    // move the head to point to the new node we just created
    (*head_ref) = new_node;

    return node_index; // ID of this node
}

void delete_node(SessionInfoNode_T** head_ref, int ID) {
    node_index--;
    SessionInfoNode_T* current = *head_ref, *prev;
    while (current != NULL && current->ID == ID) {
        *head_ref = current->NEXT;
        free(current);
        printf("Deleted node with ID : %d\n", ID);
        return;
    }
    while (current != NULL && current->ID != ID) {
        prev = current;
        current = current->NEXT;
    }

    // If key was not present in linked list 
    if (current == NULL) {
        LOGF_ERROR(global_thl, 0, "Node that was going to be deleted does not exist", "printf");
        return;
    }

    prev->NEXT = current->NEXT;
    free(current);
    LOGF_DEBUG(global_thl, 0, "Deleted node with ID : %d", ID, "printf");
}

SessionInfoNode_T* find_node(SessionInfoNode_T* head, char *CONNECT_CODE, char *THREAD_IP, char *HWID, int ID) {
    SessionInfoNode_T* current = head;

    bool hasran = false;

    if (ID != NO_ID && hasran == false) { // if we passed an actual ID, then search for ID
        hasran = true;
        while (current != NULL)  {
           if(current->ID == ID) {
                return current; // returns pointer of the node that has the ID
                current = current->NEXT;
           }
           else {
              break;
           }
        }
        LOGF_DEBUG(global_thl, 0, "No ID matches found in any of the nodes", "printf");
    }

    if(strcmp(HWID, NULLSTRING) != 0 && hasran == false) {
        hasran = true;

        while (current != NULL) {
            if(strcmp(current->HWID, HWID) == 0) {
              return current; // returns pointer of the node that has HWID
              current = current->NEXT;
          }
          else {
              break;
          }
        }
    }
    
    if (strcmp(CONNECT_CODE, NULLSTRING) != 0 && hasran == false) { // if we passed a connect code
        hasran = true;
        while (current != NULL) {
          if(strcmp(current->CONNECT_CODE, CONNECT_CODE) == 0) {
              return current; // returns pointer of the node that has ID
              current = current->NEXT;
          }
          else {
              break;
          }
        }
        LOGF_DEBUG(global_thl, 0, "No connect code matches found in any of the nodes", "printf");
    }

    if (strcmp(THREAD_IP, NULLSTRING) != 0 && hasran == false) { // if we passed an ip address
        hasran = true;
        while (current != NULL) {
          if(strcmp(current->THREAD_IP, THREAD_IP) == 0) {
              return current; // returns pointer of the node that has IP
              current = current->NEXT;
          }
          else {
              break;
          }
        }
        LOGF_DEBUG(global_thl, 0, "No IP matches found in any of the nodes", "printf");
    }

    return NULL; // if not found return nothing
}

void print_list(SessionInfoNode_T* head) {
    SessionInfoNode_T* current = head;

    while(current != NULL) {
        printf("\n");
        printf("THREAD IP : %s \n", current->THREAD_IP);
        printf("HWID : %s \n", current->HWID);
        printf("CONNECTION_TID : %d \n", current->CONNECTION_TID);

        printf("CONNECT_CODE : %s \n", current->CONNECT_CODE);
        if(current->STATUS == READY)
            printf("STATUS : READY \n");
        else if(current->STATUS == BUSY)
            printf("STATUS : BUSY \n");
        printf("ID : %d \n", current->ID);
        
        current = current->NEXT;
    }
}

int session_info_main() {
   // this is just a test function showing what we can do

   /*
       SessionInfoNode_T* head = NULL;
       SessionInfoNode_T* tmp;

       add_node(&head, "test_thread_ip1", "test_hwid1", "test_connectiontid1", NULLSTRING, NULLSTRING);
       add_node(&head, "test_thread_ip3", "test_hwid3", "test_connectiontid3", NULLSTRING, NULLSTRING);
       add_node(&head, "test_thread_ip4", "test_hwid4", "test_connectiontid4", NULLSTRING, NULLSTRING);
       add_node(&head, "test_thread_ip5", "test_hwid5", "test_connectiontid5", NULLSTRING, NULLSTRING);
       add_node(&head, "test_thread_ip2", "test_hwid2", "test_connectiontid2", "1234", NULLSTRING);
    
       tmp = find_node(head, "1234", NULLSTRING, NULLSTRING);
       if (tmp != NULL) {
           printf("ID of node containing 1234 : %d\n", tmp->ID);
           printf("IP Address : %s\n", tmp->THREAD_IP);
       }
       delete_node(&head, 5); // delete node with id 5
       print_list(head);
    */

   return 0;
}