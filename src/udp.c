/**********************************************************************************
* (c) Rui Daniel 96317, Afonso Alemao 96135
* Last Modified: 11/04/2022
*
* Name:
*   udp.c
*
* Description:
*   UDP connections
*
* Comments:
*
**********************************************************************************/

#include "udp.h"

/**********************************************************************************
 * UDP_sender()
 *
 * Arguments:   node - pointer to node that sends the message (Nodes *)
 * 				message - pointer to message to be sent (char *)
 * 				IP - IP of the receiver
 * 				PORT - PORT of the receiver
 * 			    flag - indicates if we are in the case EFND (int)
 * 				addr - struct to store the address
 * 				fd_pred - pointer to socket of tcp predecessor connection
 * 				fd_suc - pointer to socket of tcp successor connection
 * 				socket_list - pointer to receivement sockets
 * 				size_list - size of socket_list
 * 				retrasmission - flag that controls the retrasmissions 
 * 								when the ACK is not received
 * 				ACK_hability - is the node active
 * 				saveguard_efnd - check if we are in efnd
 *
 * Return: (int) if the sending is well succeded: fd
 * 				 if the sending fails: -2
 * 				 if memory error: -1
 *
 * Side effects: None
 *
 * Description: Send udp messages and wait for ACK. Controls bentry.
 *
 ********************************************************************************/

int UDP_sender (Nodes *node, char *message, char *IP, char *PORT, int flag, struct sockaddr addr, 
			   int *fd_pred, int *fd_suc, int *socket_list, int size_list, int *retrasmission, int ACK_hability, int *saveguard_efnd) {
	struct addrinfo hints, *res = NULL;
    int fd = 0, errcode = 0, readcode = 0, ret = 0;
    ssize_t n = 0;
	socklen_t addrlen = 0;
	char buffer[128 + 1];
    struct timeval timeout;      
    timeout.tv_sec = 0;
    timeout.tv_usec = 500000;

	/* Send udp message and wait for ACK */
	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		fprintf(stderr, "error: %s\n", strerror(errno));
		return -2;
	}
    
	/* Timer to deal with potential lost of udp messages */
    if (setsockopt (fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout) < 0) {
        printf("error: setsockopt failed\n");
        return -2;
    }
    if (setsockopt (fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof timeout) < 0) {
        printf("error: setsockopt failed\n");
        return -2;
    }

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET; // IPv4
	hints.ai_socktype = SOCK_DGRAM; // UDP socket

	n = getaddrinfo(IP, PORT, &hints, &res);
	if (n != 0) {
		fprintf(stderr, "error: %s\n", strerror(errno));
		if (res != NULL) {
			freeaddrinfo(res);
		}
		return -2;
	}
	n = sendto(fd, message, strlen(message), 0, res->ai_addr, res->ai_addrlen);
	if (n == -1) {
		fprintf(stderr, "error: %s\n", strerror(errno));
		if (res != NULL) {
			freeaddrinfo(res);
		}
		return -2;
	}
	if ((*retrasmission == 1)) {
		printf("Wait, the connection you are trying to make is loading.\n");
	}
	/* ACK reception */
	addrlen = sizeof(addr);
	n = recvfrom(fd, buffer, 128, 0, &addr, &addrlen);
	if (n == -1) {
		(*retrasmission)++;
		if (*retrasmission <= 3) {
			ret = UDP_sender (node, message, IP, PORT, flag, addr, fd_pred, fd_suc, socket_list, 
							 size_list, retrasmission, ACK_hability, saveguard_efnd);
		}
		else {
			freeaddrinfo(res);
			return -2;
		}
		freeaddrinfo(res);
		if (ret > 0) {
			return ret;
		}
		return -2;
	}
	buffer[n] = '\0';
	/* printf("Received: %s\n", buffer); */

	/* If case EFND, wait for EPRED */
	if (flag == 1) {
		n = recvfrom(fd, buffer, 128, 0, &addr, &addrlen);
		if (n == -1) {
			/* fprintf(stderr, "error: %s\n", strerror(errno)); */
			return -2;
		}
		buffer[n] = '\0';
		/* printf("Received: %s\n", buffer); */
		/* ACK reception */
		if (strcmp("ACK" , buffer) != 0) {
			errcode = sendto(fd, "ACK", strlen("ACK"), 0, &addr, addrlen);
			if (errcode == -1) {
				fprintf(stderr, "Error: can't send ACK\n");
			}
		}
		/* Execute EPRED */
		readcode = ReadComand(node, buffer, 2, addr, NULL, NULL, NULL, NULL, NULL, 
							 NULL, NULL, fd_pred, fd_suc, socket_list, size_list, ACK_hability, saveguard_efnd);
		if (readcode == -2) {
			fprintf(stderr, "error: Command not valid\n");
		}
		else if (readcode == 50) {
			if (res != NULL) {
				freeaddrinfo(res);
			}
			return -1;
		}
	}

	if (res != NULL) {
		freeaddrinfo(res);
	}
    return fd;
}
