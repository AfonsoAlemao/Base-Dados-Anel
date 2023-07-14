/**********************************************************************************
* (h) Rui Daniel, Afonso Alemao
* Last Modified: 11/04/2022
*
* Name:
*   aux.h
*
* Description:
*   Prototipes to auxiliar functions
*
* Comments:
**********************************************************************************/

#ifndef _AUX_H
#define _AUX_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/time.h>
#include <signal.h>
#include <errno.h>

#define Number_Nodes 32
#define max(A,B) ((A) >= (B) ? (A) : (B))

/**********************************************************************************
 * Nodes
 *
 * Description: Struct that represents the node state
 *
 ********************************************************************************/

struct nodes {
   int suc; // Successor
   int pred; // Predecessor
   int ata; // Shortcut
   int key; // Key
   char *IP;
   char *port;
   char *IPpred;
   char *IPsuc;
   char *IPata;
   char *PORTpred;
   char *PORTsuc;
   char *PORTata;
};

typedef struct nodes Nodes;

int *addlist (int *list, int size, int fd);
int CheckACKhability (Nodes *node);
void CheckShortcut (Nodes *node);
int CloserDistance(Nodes *node,  int key);
int *closelist (int *list, int size);
char *Confirm_IP_port (char *trash);
int ConfirmKey (int key);
int distance (int k, int l);
char *free_string (char *str);
Nodes Inicialize_Node (Nodes node);
char *intToAscii_space (int number);
int close_socket (int fd);
void Closes_Frees (int fd_tcp, int fd_udp, int afd, Nodes *node, struct addrinfo *res_tcp,
					struct addrinfo *res_udp, char *message, char *IPepred, char *PORTepred,
					int fd_suc, int fd_pred, int *list, int size_list);
int *removelist (int *list, int size, int fd);

#endif
