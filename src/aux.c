/**********************************************************************************
* (c) Rui Daniel 96317, Afonso Alemao 96135
* Last Modified: 11/04/2022
*
* Name:
*   aux.c
*
* Description:
*   Auxiliar functions
*
* Comments:
*
**********************************************************************************/
#include <stdlib.h>

#include "aux.h"

/**********************************************************************************
 * CheckACKhability()
 *
 * Arguments: node - node in process (Nodes *)
 *
 * Return: (int) ack_hability
 *
 * Side effects: None
 *
 * Description: Check if the node is active. 
 * 				If is not active, desable sending ack messages. 
 *
 ********************************************************************************/

int CheckACKhability (Nodes *node) {
	if (node->pred == -1 || node->suc == -1) {
		return 0;
	}
	return 1;
}

/**********************************************************************************
 * CheckShortcut()
 *
 * Arguments: node - node in process (Nodes *)
 *
 * Return: (void)
 *
 * Side effects: None
 *
 * Description: Check if the node is alone. If that's the case, 
 * 				removes shortcut.
 *
 ********************************************************************************/

 void CheckShortcut (Nodes *node) {
	if (node->pred == node->suc && node->pred == node->key && node->key != -1 && node->ata != -1) {
		printf("Shortcut to node %d removed\n", node->ata);
		node->ata = -1;
		node->IPata = free_string(node->IPata);
		node->PORTata = free_string(node->PORTata);
	}
	return;
}

/**********************************************************************************
 * free_string()
 *
 * Arguments: str - string to be freed (char *)
 *
 * Return: (char *) NULL
 *
 * Side effects: frees str
 *
 * Description: Free string
 *
 ********************************************************************************/

char *free_string (char *str) {
    if (str != NULL) {
        free(str);
    }
    return NULL;
}

/**********************************************************************************
 * close_socket()
 *
 * Arguments: fd - socket to be closed (int)
 *
 * Return: (int) 0
 *
 * Side effects: closes fd
 *
 * Description: Close a socket
 *
 ********************************************************************************/

int close_socket (int fd) {
    if (fd != 0) {
        close(fd);
    }
    return 0;
}

/**********************************************************************************
 * closelist()
 *
 * Arguments: list - pointer to receivement sockets
 * 			  size - size of list
 *
 * Return: (int *) NULL
 *
 * Side effects: closes list of sockets
 *
 * Description: Close a list of sockets
 *
 ********************************************************************************/

int *closelist (int *list, int size) {
	int i = 0;
	for (i = 0; i < size; i++) {
		list[i] = close_socket(list[i]);
	}
	return list;
}

/**********************************************************************************
 * addlist()
 *
 * Arguments: list - pointer to receivement sockets
 * 			  size - size of list
 * 			  fd - socket to be added to the list
 *
 * Return: (int *) list of sockets
 *
 * Side effects: None
 *
 * Description: Add socket fd to the list of sockets
 *
 ********************************************************************************/

int *addlist (int *list, int size, int fd) {
	int i = 0;
	if (fd == 0) {
		return list;
	}
	for (i = 0; i < size; i++) {
		if (list[i] == 0) {
			list[i] = fd;
			return list;
		}
	}
	printf("Out of memory\n"); 
	/* List of sockets in memory is full */
	return list;
}

/**********************************************************************************
 * removelist()
 *
 * Arguments: list - pointer to receivement sockets
 * 			  size - size of list
 * 			  fd - socket to be removed from the list
 *
 * Return: (int *) list of sockets
 *
 * Side effects: None
 *
 * Description: Removes socket fd from the list of sockets
 *
 ********************************************************************************/

int *removelist (int *list, int size, int fd) {
	int i = 0;
	if (fd == 0) {
		return list;
	}
	for (i = 0; i < size; i++) {
		if (list[i] == fd) {
			list[i] = 0;
			return list;
		}
	}
	return list;
}

/**********************************************************************************
 * Closes_Frees()
 *
 * Arguments: fd_tcp, fd_udp, afd - sockets to be closed (int)
 * 			  node - pointer to node with components to be freed (Nodes *)
 * 			  res_tcp, res_udp - pointers to addr info structure to be freed 
 * 			  message, IPepred, PORTepred - strings to be freed (char *)
 * 			  fd_pred - pointer to socket of tcp predecessor connection
 * 			  fd_suc - pointer to socket of tcp successor connection
 * 			  socket_list - pointer to receivement sockets
 * 			  size_list - size of socket_list
 *
 * Return: void
 *
 * Side effects: frees the input
 *
 * Description: Close and frees to ensure correct memory management
 * 
 ********************************************************************************/

void Closes_Frees (int fd_tcp, int fd_udp, int afd, Nodes *node, struct addrinfo *res_tcp,
					struct addrinfo *res_udp, char *message, char *IPepred, char *PORTepred,
					int fd_suc, int fd_pred, int *list, int size_list) {
	fd_tcp = close_socket(fd_tcp);
	fd_udp = close_socket(fd_udp);
	afd = close_socket(afd);
	fd_suc = close_socket(fd_suc);
	fd_pred = close_socket(fd_pred);

	if (res_udp != NULL) {
		freeaddrinfo(res_udp);
	}
	if (res_tcp != NULL) {
		freeaddrinfo(res_tcp);
	}
	list = closelist(list, size_list);
	if (list != NULL) {
		free(list);
	}
	node->IPpred = free_string (node->IPpred);
	node->IPsuc = free_string (node->IPsuc);
	node->PORTsuc = free_string(node->PORTsuc);
	node->PORTpred = free_string(node->PORTpred);
	node->IP = free_string(node->IP);
	node->port = free_string(node->port);
	node->IPata = free_string(node->IPata);
	node->PORTata = free_string(node->PORTata);
	message = free_string(message);
	IPepred = free_string(IPepred);
	PORTepred = free_string(PORTepred);
	return;
}

/**********************************************************************************
 * ConfirmKey()
 *
 * Arguments: key - key to be confirmed (int)
 *
 * Return: (int) key if key is valid
 * 				 -1 if key is not valid
 *
 * Side effects: None
 *
 * Description: Confirm if the key is valid: 0 <= key <= 31
 *
 ********************************************************************************/

int ConfirmKey (int key) {
	if (key >= 0 && key <= Number_Nodes - 1) {
		return key;
	}
	return -1;
}

/**********************************************************************************
 * distance()
 *
 * Arguments: k - first node to be compared (int)
 * 			  l - second node to be compared (int)
 *
 * Return: (int) distance of the nodes mod 32 = Number_Nodes
 *
 * Side effects: None
 *
 * Description: Calculates the distance beetween two nodes in the ring
 *
 ********************************************************************************/

int distance (int k, int l) {
    int aux;
    if (l > k) {
        return (l - k) % Number_Nodes;
    }
    aux = (l - k) % Number_Nodes + Number_Nodes;
    if (aux == Number_Nodes) {
        return 0;
    }
    return (l - k) % Number_Nodes + Number_Nodes;
}

/**********************************************************************************
 * Confirm_IP_port()
 *
 * Arguments: trash - string with the format: "IP" or "PORT" (char *)
 *
 * Return: (char *) if format is valid: IP or PORT
 * 					if format is invalid: NULL
 *
 * Side effects: Allocates memory for the string created
 *
 * Description: Confirm if the format that the user specifies
 * 				is valid
 *
 ********************************************************************************/

char *Confirm_IP_port (char *trash) {
    if ( (strcmp(trash, "") == 0) || trash == NULL) {
        return NULL;
    }
	int size = strlen(trash), i = 0;

	if (trash[strlen(trash) - 1] == '\n') {
		size--;
	}
	char *new;
	if (strlen(trash) <= 0) {
		return NULL;
	}
	new = (char *) calloc((size + 1) , sizeof(char));
	if (new == NULL) { /* Error */
		return NULL;
	}
	for (i = 0; i < size; i++){
		new[i] = trash[i];
	}
	return new;
}

/**********************************************************************************
 * intToAscii_space()
 *
 * Arguments: number - integer to be confirmed (int)
 *
 * Return: (char *) integer converted to string
 *
 * Side effects: Allocates memory for the string created
 *
 * Description: Converts the number to a string with a space ahead
 *
 ********************************************************************************/

char *intToAscii_space (int number) {
	char *ascii = NULL;
	char c = 48;

	if (number < 10) {
	    ascii = (char *) calloc((3) , sizeof(char));
		if (ascii == NULL) {
			return NULL;
		}
        ascii[1] = c + number;
        ascii[2] = '\0';
        ascii[0] = ' ';
	}
    else {
        ascii = (char *) calloc((4) , sizeof(char));
		if (ascii == NULL) {
			return NULL;
		}
        ascii[1] = c + (number / 10);
        ascii[2] = c + (number % 10);
        ascii[3] = '\0';
        ascii[0] = ' ';
    }
	return ascii;
}

/**********************************************************************************
 * CloserDistance()
 *
 * Arguments:   node - pointer to node in process (Nodes *)
 * 				key - the key we are checking (int)
 *
 * Return: (int) 0 : didn't work
				 1 : key is already in the node
				 2 : shorcut has closer distance = delegates the find to shortcut by UDP
				 3 : succesor has closer distance = delegates the find to successor by TCP
 *
 * Side effects: None
 *
 * Description: Checks if the key is already in the node.
 * 				Else checks who has the shortest distance to the key:
 * 				the shortcut or the successor (if they exist)
 *
 ********************************************************************************/

int CloserDistance(Nodes *node,  int key) {
	if (node->suc == -1) {
		return 0;
	}
	if (distance(node->key, key) <= distance(node->suc, key)) {
		return 1; // I already have the key
	}
	if (node->ata == -1) {
		return 3;
	}
	if (distance(node->suc, key) <= distance(node->ata, key)) {
		return 3;
	}
	else {
		return 2;
	}
	return 0;
}

/**********************************************************************************
 * Inicialize_Node()
 *
 * Arguments: node - node to be inicialized (Nodes)
 *
 * Return: (Nodes) node
 *
 * Side effects: Inicialize node
 *
 * Description: Inicialize node with insignificant values ​​with confirmation utility
 *
 ********************************************************************************/

Nodes Inicialize_Node (Nodes node) {
    node.suc = -1;
	node.pred = -1;
	node.ata = -1;
	node.IP = NULL;
	node.port = NULL;
	node.IPpred = NULL;
	node.IPsuc = NULL;
	node.IPata = NULL;
	node.PORTpred = NULL;
	node.PORTsuc = NULL;
	node.PORTata = NULL;
    return node;
}
