/**********************************************************************************
* (h) Rui Daniel, Afonso Alemao
* Last Modified: 11/04/2022
*
* Name:
*   commands.h
*
* Description:
*   Prototipes to node commands
*
* Comments: 
**********************************************************************************/

#ifndef _COM_H
#define _COM_H

#include "aux.h"
#include "tcp.h"
#include "udp.h"

void interface();
int leave (Nodes *node, char *message, int *fd_suc, int *fd_pred, int *socket_list, int size_list);
int ReadComand(Nodes *node, char *command, int flag, struct sockaddr addr_udp, int *sequence_num,
				struct sockaddr *addr_udp_epred, int *print, int *afd, int *key_epred, char **IPepred,
				char **PORTepred, int *fd_pred, int *fd_suc, int *socket_list, int size_list, int ACK_hability, int *saveguard_efnd);
                
#endif