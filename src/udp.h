/**********************************************************************************
* (h) Rui Daniel, Afonso Alemao
* Last Modified: 11/04/2022
*
* Name:
*   udp.h
*
* Description:
*   Prototipes to udp connections
*
* Comments: 
**********************************************************************************/

#ifndef _UDP_H
#define _UDP_H

#include "aux.h"
#include "commands.h"

int UDP_sender (Nodes *node, char *message, char *IP, char *PORT, int flag, struct sockaddr addr, 
			   int *fd_pred, int *fd_suc, int *socket_list, int size_list, int *retrasmission, int ACK_hability, int *saveguard_efnd);

#endif