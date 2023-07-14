/**********************************************************************************
* (c) Rui Daniel 96317, Afonso Alemao 96135
* Last Modified: 11/04/2022
*
* Name:
*   tcp.c
*
* Description:
*   TCP connections
*
* Comments:
*
**********************************************************************************/

#include "tcp.h"

/**********************************************************************************
 * TCP_newfd()
 *
 * Arguments:   node - pointer to node that connect (Nodes *)
 * 			    flag - indicates the receiver of the connection (int)
 *
 * Return: (int) if sending is well succeded: socket of new connection
 * 				 if sending fails: -1
 *
 * Side effects: Creates a new socket associated with the new connection
 *
 * Description: Connect by tcp with predecessor (flag = 0),
 * 				to successor (flag = 1) or to shortcut (flag = 2)
 *
 ********************************************************************************/

int TCP_newfd (Nodes *node, int flag) {
	if (flag > 2 || flag < 0) {
		return -1;
	}
	struct addrinfo hints, *res;
	int fd = 0, n = 0;

	fd = socket(AF_INET, SOCK_STREAM, 0); // TCP socket
	if (fd == -1) {
		fprintf(stderr, "error: %s\n", strerror(errno));
		if (res != NULL) {
			freeaddrinfo(res);
			res = NULL;
		}
		return -1;
	}
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET; // IPv4
	hints.ai_socktype = SOCK_STREAM; // TCP socket

	if (flag == 0) {
		n = getaddrinfo(node->IPpred, node->PORTpred, &hints, &res);
	}
	else if (flag == 1) {
		n = getaddrinfo(node->IPsuc, node->PORTsuc, &hints, &res);
	}
	else if (flag == 2) {
		n = getaddrinfo(node->IPata, node->PORTata, &hints, &res);
	}

	if (n != 0) {
		fprintf(stderr, "error: getaddrinfo: %s\n", gai_strerror(n));
		freeaddrinfo(res);
		close(fd);
		return -1;
	}
	n = connect(fd, res->ai_addr, res->ai_addrlen);
	if (n == -1) {
		freeaddrinfo(res);
		close(fd);
		return -1;
	}
    freeaddrinfo(res);
	return fd;
}

/**********************************************************************************
 * TCP_send()
 *
 * Arguments:   message - pointer to message to be sent (char *)
 * 			    fd - socket with the connection
 *
 * Return: (int) if sending is well succeded: 0
 * 				 if sending fails: -1
 *
 * Side effects: None
 *
 * Description: Send a TCP message
 *
 ********************************************************************************/

int TCP_send (char *message, int fd) {
	if (message == NULL) {
		return -1;
	}
	ssize_t nbytes, nleft, nwritten;
	char *ptr = NULL, buffer[128 + 1];
	ptr = strcpy(buffer, message);
	nbytes = strlen(ptr);
	nleft = nbytes;
	while (nleft > 0) {
		nwritten = write(fd, ptr, nleft);
		if (nwritten <= 0) {
			/* fprintf(stderr, "error: write: %s\n", strerror(errno)); */
			fd = close_socket(fd);
			return -1;
		}
		nleft -= nwritten;
		ptr += nwritten;
	}
	return 0;
}
