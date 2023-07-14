/**********************************************************************************
* //(h) Rui Daniel, Afonso Alemao
* Last Modified: 11/04/2022
*
* Name:
*   tcp.h
*
* Description:
*   Prototipes to tcp connections
*
* Comments: 
**********************************************************************************/

#ifndef _TCP_H
#define _TCP_H

#include "aux.h"

int TCP_send (char *message, int fd);
int TCP_newfd (Nodes *node, int flag);

#endif