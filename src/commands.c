/**********************************************************************************
* (c) Rui Daniel 96317, Afonso Alemao 96135
* Last Modified: 11/04/2022
*
* Name:
*   commands.c
*
* Description:
*   Commands received by nodes processing
*
* Comments:
*
**********************************************************************************/

#include "commands.h"

/*************************************************************************************
 * interface()
 *
 * Arguments: None
 *
 * Return: (void)
 *
 * Side effects: None
 *
 * Description: Shows interface to user
 *
 ***********************************************************************************/

void interface() {
	printf("---------------------------------------------------------------------------------------------------------------\n");
	printf(" i [interface]: Shows user interface.\n");
	printf(" n [new]: Creates a ring with just this node.\n");
	printf(" b [bentry] boot boot.IP boot.port : Entry of a node in the ring which the boot node belongs to.\n");
	printf(" p [pentry] pred pred.IP pred.port: Entry of a node in the ring knowing that the precedessor will be node predecessor.\n");
	printf(" c [chord] i i.IP i.port: Create a shortcut to node i.\n");
	printf(" echord: Eliminates the shortcut.\n");
	printf(" s [show]: Shows the node state.\n");
	printf(" f [find]: Search a key, returning the node key, IP and PORT that contains that key.\n");
	printf(" l [leave]: The node leaves the ring\n");
	printf(" e [exit]: Program shut down\n");
	printf("---------------------------------------------------------------------------------------------------------------\n");
	return;
}

/**********************************************************************************
 * leave()
 *
 * Arguments:   node - pointer to node in process (Nodes *)
 * 				message - pointer to char with allocated memory (char *)
 * 				fd_pred - pointer to socket of tcp predecessor connection
 * 				fd_suc - pointer to socket of tcp successor connection
 * 				socket_list - pointer to receivement sockets
 * 				size_list - size of socket_list
 *
 * Return: (int) leave not valid: -3
 * 				 leave works: 0
 * 				 leave error: 2
 *
 * Side effects: None
 *
 * Description: Removes a node from the ring
 *
 ********************************************************************************/

int leave (Nodes *node, char *message, int *fd_suc, int *fd_pred, int *socket_list, int size_list) { 
	char *id_char = NULL;
	int did_work = 0;
	if (node->suc == -1 && node->pred == -1 && node->ata == -1) {
		return -3;
	}

	/* Send PRED message */
	id_char = intToAscii_space(node->pred);
	if (id_char == NULL) {
		return -3;
	}
	message = strcpy(message,"PRED");
	message = strcat(message, id_char);
	message = strcat(message, " ");
	message = strcat(message, node->IPpred);
	message = strcat(message, " ");
	message = strcat(message, node->PORTpred);
	message = strcat(message, "\n");

	id_char = free_string(id_char);

	if (node->suc != -1 && node->key != node->suc) {
		/* printf("Sent the message = %s to Node: %d\n", message, node->suc); */
		did_work = TCP_send(message, *fd_suc);
		if (did_work == -1) {
			return -2;
		}
	}
	else {
		*fd_suc = close_socket(*fd_suc);
		*fd_pred = close_socket(*fd_pred);
	}
	node->IPpred = free_string(node->IPpred);
	node->PORTpred = free_string(node->PORTpred);
	node->IPsuc = free_string(node->IPsuc);
	node->PORTsuc =	free_string(node->PORTsuc);
	node->IPata = free_string(node->IPata);
	node->PORTata = free_string(node->PORTata);
	if (node->pred == node->suc) {
		*fd_pred = close_socket(*fd_pred);
		*fd_suc = *fd_pred;
	}
	node->suc = -1;
	node->pred = -1;
	node->ata = -1;
	return 0;
}

/*************************************************************************************
 * ReadComand()
 *
 * Arguments:   node - node in process (Nodes *)
 * 				command - command that is going to be processed (char *)
 * 			    flag - indicates the protocol of the command received (int)
 * 				addr_udp - contains the address of the sender of the last UDP
 * 						   received message (struct sockaddr)
 * 				sequence_num - pointer to the id of the current find process
 *				addr_udp_epred - contains the address of the sender of the last
 *							UDP EPRED received message (struct sockaddr)
 *				print - pointer to integer that controls the output
 *				afd - socket with session open (int)
 *				key_epred (int *), IPepred(char **), PORTepred(char **) - values that
 *				readcommand will insert to enable EPRED
 *				fd_pred - pointer to socket of tcp predecessor connection
 * 				fd_suc - pointer to socket of tcp successor connection
 * 				socket_list - pointer to receivement sockets
 * 				size_list - size of socket_list
 * 				ACK_hability - is the node active
 * 				saveguard_efnd - check if we are in efnd
 *
 * Return: (int) -2 : didn't work
 *				 0 : work well
 *				 60 : delegates the program shut down
 *				 99 : information the node need to send a EPRED
 *				 50 : out of memory
 *
 * Side effects: None
 *
 * Description: Process the command
 *
 ***********************************************************************************/

int ReadComand(Nodes *node, char *command, int flag, struct sockaddr addr_udp, int *sequence_num,
				struct sockaddr *addr_udp_epred, int *print, int *afd, int *key_epred, char **IPepred,
				char **PORTepred, int *fd_pred, int *fd_suc, int *socket_list, int size_list, int ACK_hability, int *saveguard_efnd) {
	char command_first_word[20] = "", *IP_origin_key = NULL, *PORT_origin_key = NULL;
	int i = 0, did_work = 0, seq_aux = 0, key_to_find = 0, error_code = 0, salvaguarda_aux = 0;
	char string_IPpred[40] = "", string_PORTpred[40] = "", *salvaguarda_IP = NULL, *salvaguarda_PORT = NULL;
	char string_IPsuc[40] = "", string_PORTsuc[40] = "";
	char string_IPata[40] = "", string_PORTata[40] = "";
	char string_IP_origin_key[40] = "", string_PORT_origin_key[40] = "";
	int aux = -1, origin_key = 0, flag_find = 0, udp_error = 0;
	char *message = NULL, *id_char = NULL, *id_char1 = NULL, *id_char2 = NULL, *auxiliar1 = NULL, *auxiliar2 = NULL;
	int confirm_pentry = 0, zero = 0, flag_ret = 0;
    message = (char *) calloc((128 + 1) , sizeof(char));
	if (message == NULL) {
		return 50;
	}
	for (i = 0; i < strlen(command) - 1; i++) {
		command_first_word[i] = command[i];
		if (command[i + 1] == ' ') {
			i = strlen(command);
		}
	}

	switch (command_first_word[0]) {
		/* interface - displays user interface */
		case 'i':
			/* Confirm if command is valid */
			if ((strcmp(command_first_word, "interface") != 0) && (strlen(command_first_word) != 1)) {
				message = free_string(message);
				return -2;
			}
			
			interface();
			
			break;
		
		/* new: create a ring with just the node */
		case 'n':
			/* Confirm if command is valid */
			if ((strcmp(command_first_word, "new") != 0) && (strlen(command_first_word) != 1)) {
				message = free_string(message);
				return -2;
			}
			if (node->suc != -1 || node->pred != -1 || node->ata != -1) {
                printf("Node already in the ring\n");
				message = free_string(message);
				return -2;
			}
			node->suc = node->key;
			node->pred = node->key;
			node->IPpred = (char *) calloc((128) , sizeof(char));
			if (node->IPpred == NULL) {
				printf("Error, out of memory!\n");
				message = free_string(message);
				return 50;
			}
            strcpy(node->IPpred, node->IP);

			node->IPsuc = (char *) calloc((128) , sizeof(char));
			if (node->IPsuc == NULL) {
				node->IPpred = free_string(node->IPpred);
				printf("Error, out of memory!\n");
				message = free_string(message);
				return 50;
			}
            strcpy(node->IPsuc, node->IP);

			node->PORTpred = (char *) calloc((128) , sizeof(char));
			if (node->PORTpred == NULL) {
				node->IPpred = free_string(node->IPpred);
				node->IPsuc = free_string(node->IPsuc);
				printf("Error, out of memory!\n");
				message = free_string(message);
				return 50;
			}
            strcpy(node->PORTpred, node->port);

			node->PORTsuc = (char *) calloc((128) , sizeof(char));
			if (node->PORTsuc == NULL) {
				node->IPpred = free_string(node->IPpred);
				node->IPsuc = free_string(node->IPsuc);
				node->PORTpred = free_string(node->PORTpred);
				printf("Error, out of memory!\n");
				message = free_string(message);
				return 50;
			}
            strcpy(node->PORTsuc, node->port);

			break;

		/* bentry: Node entry in the ring to which this node belongs */
		case 'b':
			/* Confirm if command is valid */
			if ((strcmp(command_first_word, "bentry") != 0) && (strlen(command_first_word) != 1)) {
				message = free_string(message);
				return -2;
			}

			sscanf(command, "%s %d %s %s", command_first_word, &(origin_key), string_IP_origin_key, string_PORT_origin_key);

			if (node->key == origin_key) {
				printf("Bentry not valid\n");
				message = free_string(message);
				return -2;
			}

			if (ConfirmKey(origin_key) == -1) {
				message = free_string(message);
				return -2;
			}

			IP_origin_key = Confirm_IP_port(string_IP_origin_key);
			if (IP_origin_key == NULL) {
                message = free_string(message);
				return -2;
			}

			PORT_origin_key = Confirm_IP_port(string_PORT_origin_key);
			if (PORT_origin_key == NULL) {
			    IP_origin_key = free_string(IP_origin_key);
                message = free_string(message);
				return -2;
			}

			/* Send EFND k by tcp with k = my key */
			id_char = intToAscii_space(node->key);
			if (id_char == NULL) {
				IP_origin_key = free_string(IP_origin_key);
				PORT_origin_key = free_string(PORT_origin_key);
                message = free_string(message);
				return -2;
			}

			message = strcpy(message,"EFND");
			message = strcat(message, id_char);

			did_work = UDP_sender(node, message, IP_origin_key, PORT_origin_key, 1, *addr_udp_epred, 
								 fd_pred, fd_suc, socket_list, size_list, &zero, ACK_hability, saveguard_efnd);
			if (did_work == -2 || did_work == -1) {
				message = free_string(message);
				id_char = free_string(id_char);
				IP_origin_key = free_string(IP_origin_key);
				PORT_origin_key = free_string(PORT_origin_key);
				if (did_work == -1) {
					return 50;
				}
				return -2;
			}
			*afd = close_socket(*afd);
			*afd = did_work;
			id_char = free_string(id_char);
			*addr_udp_epred = addr_udp;
			IP_origin_key = free_string(IP_origin_key);
            PORT_origin_key = free_string(PORT_origin_key);
			break;

		/* pentry: Entry of a node in the ring knowing that the precedessor will be node predecessor */
		case 'p':
			/* Confirm if command is valid */
			if ((strcmp(command_first_word, "pentry") != 0) && (strlen(command_first_word) != 1)) {
				message = free_string(message);
				return -2;
			}
			sscanf(command, "%s %d %s %s", command_first_word, &(aux), string_IPpred, string_PORTpred);

			if (node->suc != -1 || node->suc != -1 || node->key == aux) {
				printf("Pentry not valid\n");
				message = free_string(message);
				return -2;
			}
            if (node->suc != -1) {
                if (aux >= node->suc || aux <= node->key) {
                    printf("Pentry not valid\n");
                    message = free_string(message);
                    return -2;
                }
            }

			/* Saveguard node->pred */
			salvaguarda_aux = node->pred;
			if (ConfirmKey(aux) == -1) {
				message = free_string(message);
				return -2;
			}
            node->pred = aux;

			/* Saveguard node->IPpred, node->PORTpred */
			if (node->IPpred != NULL) {
				salvaguarda_IP = (char *) calloc((strlen(node->IPpred) + 1) , sizeof(char));
				if (salvaguarda_IP == NULL) {
					printf("Error, out of memory!\n");
					message = free_string(message);
					return 50;
				}
				strcpy(salvaguarda_IP, node->IPpred);
			}
			if (node->PORTpred != NULL) {
				salvaguarda_PORT = (char *) calloc((strlen(node->PORTpred) + 1) , sizeof(char));
				if (salvaguarda_PORT == NULL) {
					printf("Error, out of memory!\n");
					salvaguarda_IP = free_string(salvaguarda_IP);
					message = free_string(message);
					return 50;
				}
				strcpy(salvaguarda_PORT, node->PORTpred);
			}

			node->IPpred = free_string(node->IPpred);
			node->PORTpred = free_string(node->PORTpred);

			node->IPpred = Confirm_IP_port(string_IPpred);
			if (node->IPpred == NULL) {
				/* Restitution node->pred */
				node->pred = salvaguarda_aux;

				/* Restitution node->IP_pred */
				node->IPpred = free_string(node->IPpred);
				if (salvaguarda_IP != NULL) {
					node->IPpred = (char *) calloc((strlen(salvaguarda_IP) + 1) , sizeof(char));
					if (node->IPpred == NULL) {
						printf("Error, out of memory!\n");
						message = free_string(message);
						salvaguarda_IP = free_string(salvaguarda_IP);
						salvaguarda_PORT = free_string(salvaguarda_PORT);
						return 50;
					}
					strcpy(node->IPpred, salvaguarda_IP);
				}

				message = free_string(message);
				salvaguarda_IP = free_string(salvaguarda_IP);
				salvaguarda_PORT = free_string(salvaguarda_PORT);
				return -2;
			}

			node->PORTpred = Confirm_IP_port(string_PORTpred);
			if (node->PORTpred == NULL) {
				/* Restitution node->pred */
				node->pred = salvaguarda_aux;

				/* Restitution node->IP_pred */
				node->IPpred = free_string(node->IPpred);
				if (salvaguarda_IP != NULL) {
					node->IPpred = (char *) calloc((strlen(salvaguarda_IP) + 1) , sizeof(char));
					if (node->IPpred == NULL) {
						printf("Error, out of memory!\n");
						salvaguarda_IP = free_string(salvaguarda_IP);
						salvaguarda_PORT= free_string(salvaguarda_PORT);
						message = free_string(message);
						return 50;
					}
					strcpy(node->IPpred, salvaguarda_IP);
				}

				/* Restitution node->PORT_pred */
				node->PORTpred = free_string(node->PORTpred);
				if (salvaguarda_PORT != NULL) {
					node->PORTpred = (char *) calloc((strlen(salvaguarda_PORT) + 1) , sizeof(char));
					if (node->PORTpred == NULL) {
						printf("Error, out of memory!\n");
						salvaguarda_IP = free_string(salvaguarda_IP);
						salvaguarda_PORT= free_string(salvaguarda_PORT);
						message = free_string(message);
						return 50;
					}
					strcpy(node->PORTpred, salvaguarda_PORT);
				}

				message = free_string(message);
				salvaguarda_IP = free_string(salvaguarda_IP);
				salvaguarda_PORT = free_string(salvaguarda_PORT);
				return -2;
			}

			if (node->pred != node->suc) {
				if (*fd_suc != *fd_pred) {
					*fd_pred = close_socket(*fd_pred);
				}
				*fd_pred = TCP_newfd (node, 0);
			}
			else {
				if (*fd_suc != *fd_pred) {
					*fd_pred = close_socket(*fd_pred);
				}
				*fd_pred = *fd_suc;
			}

			salvaguarda_IP = free_string(salvaguarda_IP);
			salvaguarda_PORT = free_string(salvaguarda_PORT);

			/* Send SELF to predecessor by TCP */
			id_char = intToAscii_space(node->key);
			if (id_char == NULL) {
                message = free_string(message);
				return -2;
			}

			message = strcpy(message,"SELF");
			message = strcat(message, id_char);
			message = strcat(message, " ");
			message = strcat(message, node->IP);
			message = strcat(message, " ");
			message = strcat(message, node->port);
			message = strcat(message, "\n");

			/* If you send pentry to yourself, that will be no effect */
			if (node->key != node->pred) {
				/* printf("Sent the message = %s to Node: %d\n", message, node->pred); */
				did_work = TCP_send(message, *fd_pred);
				if (did_work == -1) {
					message = free_string(message);
					return -2;
				}
			}
			id_char = free_string(id_char);
			break;
		
		/* chord: Create a shortcut to node */
		case 'c':
			/* Confirm if command is valid */
			if ((strcmp(command_first_word, "chord") != 0) && (strlen(command_first_word) != 1)) {
				message = free_string(message);
				return -2;
			}
			if (node->suc == -1 || node->pred == -1) {
                printf("Node is not in the ring\n");
                message = free_string(message);
				return -2;
			}
			sscanf(command, "%s %d %s %s", command_first_word, &(aux), string_IPata, string_PORTata);

			/* Saveguard node->ata */
			salvaguarda_aux = node->ata;
			node->ata = ConfirmKey(aux);
			if (node->ata == -1) {
				node->ata = salvaguarda_aux;
				message = free_string(message);
				return -2;
			}

			/* Saveguard node->IPata, node->PORTata */
			if (node->IPata != NULL) {
				salvaguarda_IP = (char *) calloc((strlen(node->IPata) + 1) , sizeof(char));
				if (salvaguarda_IP == NULL) {
					printf("Error, out of memory!\n");
					salvaguarda_IP = free_string(salvaguarda_IP);
					salvaguarda_PORT= free_string(salvaguarda_PORT);
					message = free_string(message);
					return 50;
				}
				strcpy(salvaguarda_IP, node->IPata);
			}
			if (node->PORTata != NULL) {
				salvaguarda_PORT = (char *) calloc((strlen(node->PORTata) + 1) , sizeof(char));
				if (salvaguarda_PORT == NULL) {
					printf("Error, out of memory!\n");
					salvaguarda_IP = free_string(salvaguarda_IP);
					salvaguarda_PORT= free_string(salvaguarda_PORT);
					message = free_string(message);
					return 50;
				}
				strcpy(salvaguarda_PORT, node->PORTata);
			}

			node->IPata = free_string(node->IPata);
			node->PORTata = free_string(node->PORTata);

			node->IPata = Confirm_IP_port(string_IPata);
			if (node->IPata == NULL) {
				/* Restitution node->ata */
				node->ata = salvaguarda_aux;

				/* Restitution node->IP_ata */
				node->IPata = free_string(node->IPata);
				if (salvaguarda_IP != NULL) {
					node->IPata = (char *) calloc((strlen(salvaguarda_IP) + 1) , sizeof(char));
					if (node->IPata == NULL) {
						printf("Error, out of memory!\n");
						salvaguarda_IP = free_string(salvaguarda_IP);
						salvaguarda_PORT= free_string(salvaguarda_PORT);
						message = free_string(message);
						return 50;
					}
					strcpy(node->IPata, salvaguarda_IP);
				}

				salvaguarda_IP = free_string(salvaguarda_IP);
				salvaguarda_PORT = free_string(salvaguarda_PORT);
				message = free_string(message);
				return -2;
			}
			node->PORTata = Confirm_IP_port(string_PORTata);
			if (node->PORTata == NULL) {
                /* Restitution node->ata */
				node->ata = salvaguarda_aux;

				/* Restitution node->IP_ata */
				node->IPata = free_string(node->IPata);
				if (salvaguarda_IP != NULL) {
					node->IPata = (char *) calloc((strlen(salvaguarda_IP) + 1) , sizeof(char));
					if (node->IPata == NULL) {
						printf("Error, out of memory!\n");
						salvaguarda_IP = free_string(salvaguarda_IP);
						salvaguarda_PORT= free_string(salvaguarda_PORT);
						message = free_string(message);
						return 50;
					}
					strcpy(node->IPata, salvaguarda_IP);
				}

				/* Restitution node->PORT_ata */
				node->PORTata = free_string(node->PORTata);
				if (salvaguarda_PORT != NULL) {
					node->PORTata = (char *) calloc((strlen(salvaguarda_PORT) + 1) , sizeof(char));
					if (node->PORTata == NULL) {
						printf("Error, out of memory!\n");
						salvaguarda_IP = free_string(salvaguarda_IP);
						salvaguarda_PORT= free_string(salvaguarda_PORT);
						message = free_string(message);
						return 50;
					}
					strcpy(node->PORTata, salvaguarda_PORT);
				}

				salvaguarda_IP = free_string(salvaguarda_IP);
				salvaguarda_PORT = free_string(salvaguarda_PORT);
				message = free_string(message);
				return -2;
			}
			salvaguarda_IP = free_string(salvaguarda_IP);
			salvaguarda_PORT = free_string(salvaguarda_PORT);
			if (salvaguarda_aux != -1) {
				printf("Chord to node %d eliminated!\n", salvaguarda_aux);
			}
			break;
		
		/* show: Shows the node state */
		case 's':
			/* Confirm if command is valid */
			if ((strcmp(command_first_word, "show") != 0) && (strlen(command_first_word) != 1)) {
				message = free_string(message);
				return -2;
			}
			/* Invalid pentry */
			if ((node->suc != -1 && node->pred == -1) || (node->suc == -1 && node->pred != -1)) {
				node->suc = -1;
				*fd_suc = close_socket(*fd_suc);
				node->pred = -1;
				*fd_pred = close_socket(*fd_pred);
			}
			
			printf("Node status: \n");
            printf("Node key: %d        | Node IP: %s        | Node PORT: %s \n", node->key, node->IP, node->port);

            if (node->pred != -1) {
                printf("Predecessor key: %d | Predecessor IP: %s | Predecessor PORT: %s \n", node->pred, node->IPpred, node->PORTpred);
            }
            else {
				printf("No predecessor \n");
			}
			if (node->suc != -1) {
                printf("Sucessor key: %d    | Sucessor IP: %s    | Sucessor PORT: %s \n", node->suc, node->IPsuc, node->PORTsuc);
            }
            else {
				printf("No sucessor \n");
			}
            if (node->ata != -1) {
                printf("Shortcut key: %d    | Shortcut IP: %s    | Shortcut PORT: %s \n", node->ata, node->IPata, node->PORTata);
            }
            else {
				printf("No Shortcut \n");
			}
			break;
		
		/* find: Search a key, returning the node key, IP and PORT that contains that key */
		case 'f':
			/* Confirm if command is valid */
			if ((strcmp(command_first_word, "find") != 0) && (strlen(command_first_word) != 1)) {
				message = free_string(message);
				return -2;
			}

			/* Final result in stdin */
			*print = 1;

			sscanf(command, "%s %d", command_first_word, &(aux));

			/* flag_find = 0 (didn't work)
			flag find = 1 (key already in the node)
			flag_find = 2 (delegate the find to ata UDP)
			flag_find = 3 (delegate the find to suc TCP) */

			if (aux < 0 || aux > Number_Nodes - 1) {
				message = free_string(message);
				printf("The node that you are searching if invalid.\nTry to find a node between 0 and 31\n");
				return -2;
			}

			flag_find = CloserDistance(node, aux);
			if (flag_find == 1) {
				if (*print == 1) {
					printf("key %d is in: \n    Node key: %d        | Node IP: %s        | Node PORT: %s \n", aux, node->key, node->IP, node->port);
				}
				else {
                    *IPepred = free_string(*IPepred);
                    *PORTepred = free_string(*PORTepred);
					message = free_string(message);
					*key_epred = node->key;
					*IPepred = (char *) calloc((strlen(node->IP) + 1) , sizeof(char));
					if (*IPepred == NULL) {
						printf("Error, out of memory!\n");
						message = free_string(message);
						return 50;
					}
					strcpy(*IPepred, node->IP);
					*PORTepred = (char *) calloc((strlen(node->port) + 1) , sizeof(char));
					if (*PORTepred == NULL) {
						printf("Error, out of memory!\n");
						message = free_string(message);
						return 50;
					}
					strcpy(*PORTepred, node->port);
					return 99;
				}
			}
			
			/* Send FND message */
			else if (flag_find == 2) {
				id_char1 = intToAscii_space(aux);
				if (id_char1 == NULL) {
					message = free_string(message);
					return -2;
				}
				id_char2 = intToAscii_space(*sequence_num);
				if (id_char2 == NULL) {
					id_char1 = free_string(id_char1);
					message = free_string(message);
					return -2;
				}
				(*sequence_num)++;
				id_char = intToAscii_space(node->key);
				if (id_char == NULL) {
					id_char1 = free_string(id_char1);
					id_char2 = free_string(id_char2);
					message = free_string(message);
					return -2;
				}
				message = strcpy(message,"FND");
				message = strcat(message, id_char1);
				message = strcat(message, id_char2);
				message = strcat(message, id_char);
				message = strcat(message, " ");
				message = strcat(message, node->IP);
				message = strcat(message, " ");
				message = strcat(message, node->port);

				id_char1 = free_string(id_char1);
				id_char2 = free_string(id_char2);
				id_char = free_string(id_char);

				udp_error = UDP_sender(node, message, node->IPata, node->PORTata, 2, *addr_udp_epred,
											fd_pred, fd_suc, socket_list, size_list, &zero, ACK_hability, saveguard_efnd);
				if (udp_error == -2) {
					if (node->ata != -1) {
						printf("Shortcut to node %d removed\n", node->ata);
						node->ata = -1;
						node->IPata = free_string(node->IPata);
						node->PORTata = free_string(node->PORTata);
					}
                    message = free_string(message);
                    flag_ret = 1;
				}
				else if (udp_error == -1) {
					message = free_string(message);
					return 50;
				}
				else {
					*afd = close_socket(*afd);
					*afd = udp_error;
					message = free_string(message);
				}
			}
			else if (flag_find == 3) {
				id_char1 = intToAscii_space(aux);
				if (id_char1 == NULL) {
					message = free_string(message);
					return -2;
				}
				id_char2 = intToAscii_space(*sequence_num);
				if (id_char2 == NULL) {
					id_char1 = free_string(id_char1);
					message = free_string(message);
					return -2;
				}
				(*sequence_num)++;
				id_char = intToAscii_space(node->key);
				if (id_char == NULL) {
					id_char1 = free_string(id_char1);
					id_char2 = free_string(id_char2);
					message = free_string(message);
					return -2;
				}
				message = strcpy(message,"FND");
				message = strcat(message, id_char1);
				message = strcat(message, id_char2);
				message = strcat(message, id_char);
				message = strcat(message, " ");
				message = strcat(message, node->IP);
				message = strcat(message, " ");
				message = strcat(message, node->port);
				message = strcat(message, "\n");

				id_char1 = free_string(id_char1);
				id_char2 = free_string(id_char2);
				id_char = free_string(id_char);

				/* printf("Sent the message = %s to Node:%d\n", message, node->suc); */
				did_work = TCP_send (message, *fd_suc);
				if (did_work == -1) {
					message = free_string(message);
					return -2;
				}
				message = free_string(message);
			}
			else {
                message = free_string(message);
				return -2;
			}

			if (flag_ret == 1) {
				message = (char *) calloc((128 + 1) , sizeof(char));
				if (message == NULL) {
					return -2;
				}
				flag_ret = 0;
				id_char1 = intToAscii_space(aux);
				if (id_char1 == NULL) {
					message = free_string(message);
					return -2;
				}
				id_char2 = intToAscii_space(*sequence_num);
				if (id_char2 == NULL) {
					id_char1 = free_string(id_char1);
					message = free_string(message);
					return -2;
				}
				(*sequence_num)++;
				id_char = intToAscii_space(node->key);
				if (id_char == NULL) {
					id_char1 = free_string(id_char1);
					id_char2 = free_string(id_char2);
					message = free_string(message);
					return -2;
				}
				message = strcpy(message,"FND");
				message = strcat(message, id_char1);
				message = strcat(message, id_char2);
				message = strcat(message, id_char);
				message = strcat(message, " ");
				message = strcat(message, node->IP);
				message = strcat(message, " ");
				message = strcat(message, node->port);
				message = strcat(message, "\n");

				id_char1 = free_string(id_char1);
				id_char2 = free_string(id_char2);
				id_char = free_string(id_char);

				/* printf("Sent the message = %s to Node:%d\n", message, node->suc); */
				did_work = TCP_send (message, *fd_suc);
				if (did_work == -1) {
					message = free_string(message);
					return -2;
				}
				message = free_string(message);
			}

			message = free_string(message);
			/*find k
			Procura da chave k, retornando a chave, o endereço IP e o porto do nó à qual a
			chave pertence.
			*/
			break;
		
		/* leave: The node leaves the ring */
		case 'l':
			/* Confirm if command is valid */
			if ((strcmp(command_first_word, "leave") != 0) && (strlen(command_first_word) != 1)) {
				message = free_string(message);
				return -2;
			}
			error_code = leave(node, message, fd_suc, fd_pred, socket_list, size_list);
			if (error_code == -2) {
				message = free_string(message);
				return -2;
			}
			else if (error_code == -3) {
				printf("Node already out of the ring\n");
				message = free_string(message);
				return -2;
			}

			break;
		case 'e':
			/* Confirm if command is valid */
			if (((strcmp(command_first_word, "exit") != 0) && (strcmp(command_first_word, "echord") != 0)) && (strlen(command_first_word) != 1)) {
				message = free_string(message);
				return -2;
			}

			/* exit: Program shut down */
			if ((strcmp(command_first_word, "exit") == 0) || (strlen(command_first_word) == 1)) {
				error_code = leave(node, message, fd_suc, fd_pred, socket_list, size_list);
				if (error_code == -2) {
					message = free_string(message);
					return -2;
				}
				message = free_string(message);
				return 60;
			}

			/* echord: Eliminates the shortcut */
			else if (strcmp(command_first_word, "echord") == 0) {
				if (node->ata == -1) {
					printf("Shortcut already does not exist\n");
				}
				node->ata = -1;
				node->IPata = free_string(node->IPata);
				node->PORTata = free_string(node->PORTata);
			}
			break;

		/* FND: A node delegates to its successor the search for the key k that was initiated
		by node i with sequence number n*/
		case 'F':
			/* Confirm if command is valid */
			if ((strcmp(command_first_word, "FND") != 0) && (strlen(command_first_word) != 1)) {
				message = free_string(message);
				return -2;
			}

			sscanf(command, "%s %d %d %d %s %s", command_first_word, &key_to_find, &seq_aux, &origin_key, string_IP_origin_key, string_PORT_origin_key);

			IP_origin_key = Confirm_IP_port(string_IP_origin_key);
			if (IP_origin_key == NULL) {
                message = free_string(message);
				return -2;
			}
			PORT_origin_key = Confirm_IP_port(string_PORT_origin_key);
			if (PORT_origin_key == NULL) {
				IP_origin_key = free_string(IP_origin_key);
                message = free_string(message);
				return -2;
			}

			/* flag_find = 0 (didn't work)
			flag find = 1 (key already in the node)
			flag_find = 2 (delegate the find to ata UDP)
			flag_find = 3 (delegate the find to suc TCP) */
			flag_find = CloserDistance(node, key_to_find);
			if (flag_find == 1) {
				if (origin_key == node->key) {
					if (*print == 1) {
						printf("Key %d is in: \n    Node key: %d        | Node IP: %s        | Node PORT: %s \n", key_to_find, node->key, node->IP, node->port);
					}
					else {
                        *IPepred = free_string(*IPepred);
                        *PORTepred = free_string(*PORTepred);
						*key_epred = node->key;
						*IPepred = (char *) calloc((strlen(node->IP) + 1) , sizeof(char));
						if (*IPepred == NULL) {
							printf("Error, out of memory!\n");
							message = free_string(message);
							return 50;
						}
						strcpy(*IPepred, node->IP);
						*PORTepred = (char *) calloc((strlen(node->port) + 1) , sizeof(char));
						if (*PORTepred == NULL) {
							printf("Error, out of memory!\n");
							message = free_string(message);
							return 50;
						}
						strcpy(*PORTepred, node->port);
						message = free_string(message);
						return 99;
					}
				}
				else {
					/* Send RSP message */
					flag_find = CloserDistance(node, origin_key);
					id_char1 = intToAscii_space(origin_key);
					if (id_char1 == NULL) {
						message = free_string(message);
						return -2;
					}
					id_char2 = intToAscii_space(seq_aux);
					if (id_char2 == NULL) {
						id_char1 = free_string(id_char1);
						message = free_string(message);
						return -2;
					}
					id_char = intToAscii_space(node->key);
					if (id_char == NULL) {
						id_char1 = free_string(id_char1);
						id_char2 = free_string(id_char2);
						message = free_string(message);
						return -2;
					}
					message = strcpy(message,"RSP");
					message = strcat(message, id_char1);
					message = strcat(message, id_char2);
					message = strcat(message, id_char);
					message = strcat(message, " ");
					message = strcat(message, node->IP);
					message = strcat(message, " ");
					message = strcat(message, node->port);

					id_char1 = free_string(id_char1);
					id_char2 = free_string(id_char2);
					id_char = free_string(id_char);

					if (flag_find == 2) {
						udp_error = UDP_sender(node, message, node->IPata, node->PORTata, 2, *addr_udp_epred, 
											  fd_pred, fd_suc, socket_list, size_list, &zero, ACK_hability, saveguard_efnd);
						if (udp_error == -2) {
							if (node->ata != -1) {
								printf("Shortcut to node %d removed\n", node->ata);
								node->ata = -1;
								node->IPata = free_string(node->IPata);
								node->PORTata = free_string(node->PORTata);
							}
							message = free_string(message);
							flag_ret = 1;
						}
						else if (udp_error == -1) {
							message = free_string(message);
							return 50;
						}
						else {
							*afd = close_socket(*afd);
							*afd = udp_error;
						}
					}
					else if (flag_find == 3) {
						message = strcat(message, "\n");
						/* printf("Sent the message = %s to Node:%d\n", message, node->suc); */
						did_work = TCP_send (message, *fd_suc);
						if (did_work == -1) {
							message = free_string(message);
							IP_origin_key = free_string(IP_origin_key);
							PORT_origin_key = free_string(PORT_origin_key);
							return -2;
						}
					}
					
					if (flag_ret == 1) {
						message = (char *) calloc((128 + 1) , sizeof(char));
						if (message == NULL) {
							return -2;
						}
						flag_ret = 0;
						id_char1 = intToAscii_space(origin_key);
						if (id_char1 == NULL) {
							message = free_string(message);
							return -2;
						}
						id_char2 = intToAscii_space(seq_aux);
						if (id_char2 == NULL) {
							id_char1 = free_string(id_char1);
							message = free_string(message);
							return -2;
						}
						id_char = intToAscii_space(node->key);
						if (id_char == NULL) {
							id_char1 = free_string(id_char1);
							id_char2 = free_string(id_char2);
							message = free_string(message);
							return -2;
						}
						message = strcpy(message,"RSP");
						message = strcat(message, id_char1);
						message = strcat(message, id_char2);
						message = strcat(message, id_char);
						message = strcat(message, " ");
						message = strcat(message, node->IP);
						message = strcat(message, " ");
						message = strcat(message, node->port);

						id_char1 = free_string(id_char1);
						id_char2 = free_string(id_char2);
						id_char = free_string(id_char);
						message = strcat(message, "\n");
						/* printf("Sent the message = %s to Node:%d\n", message, node->suc); */
						did_work = TCP_send (message, *fd_suc);
						if (did_work == -1) {
							message = free_string(message);
							IP_origin_key = free_string(IP_origin_key);
							PORT_origin_key = free_string(PORT_origin_key);
							return -2;
						}
					}
					IP_origin_key = free_string(IP_origin_key);
					PORT_origin_key = free_string(PORT_origin_key);
					message = free_string(message);
				}
			}
			else if (flag_find == 2) {
				/* Send FND message */
				id_char1 = intToAscii_space(key_to_find);
				if (id_char1 == NULL) {
					message = free_string(message);
					return -2;
				}
				id_char2 = intToAscii_space(seq_aux);
				if (id_char2 == NULL) {
					id_char1 = free_string(id_char1);
					message = free_string(message);
					return -2;
				}
				id_char = intToAscii_space(origin_key);
				if (id_char == NULL) {
					id_char1 = free_string(id_char1);
					id_char2 = free_string(id_char2);
					message = free_string(message);
					return -2;
				}
				message = strcpy(message,"FND");
				message = strcat(message, id_char1);
				message = strcat(message, id_char2);
				message = strcat(message, id_char);
				message = strcat(message, " ");
				message = strcat(message, IP_origin_key);
				message = strcat(message, " ");
				message = strcat(message, PORT_origin_key);

				id_char1 = free_string(id_char1);
				id_char2 = free_string(id_char2);
				id_char = free_string(id_char);

				udp_error = UDP_sender(node, message, node->IPata, node->PORTata, 2, *addr_udp_epred, 
									  fd_pred, fd_suc, socket_list, size_list, &zero, ACK_hability, saveguard_efnd);
				if (udp_error == -2) {
					if (node->ata != -1) {
						printf("Shortcut to node %d removed\n", node->ata);
						node->ata = -1;
						node->IPata = free_string(node->IPata);
						node->PORTata = free_string(node->PORTata);
					}
                    message = free_string(message);
                    flag_ret = 1;
				}
				else if (udp_error == -1) {
					message = free_string(message);
					return 50;
				}
				else {
					*afd = close_socket(*afd);
					*afd = udp_error;
					message = free_string(message);
				}
			}
			else if (flag_find == 3) {
				/* Send FND message */
				id_char1 = intToAscii_space(key_to_find);
				if (id_char1 == NULL) {
					message = free_string(message);
					return -2;
				}
				id_char2 = intToAscii_space(seq_aux);
				if (id_char2 == NULL) {
					id_char1 = free_string(id_char1);
					message = free_string(message);
					return -2;
				}
				id_char = intToAscii_space(origin_key);
				if (id_char == NULL) {
					id_char1 = free_string(id_char1);
					id_char2 = free_string(id_char2);
					message = free_string(message);
					return -2;
				}
				message = strcpy(message,"FND");
				message = strcat(message, id_char1);
				message = strcat(message, id_char2);
				message = strcat(message, id_char);
				message = strcat(message, " ");
				message = strcat(message, IP_origin_key);
				message = strcat(message, " ");
				message = strcat(message, PORT_origin_key);
				message = strcat(message, "\n");

				id_char1 = free_string(id_char1);
				id_char2 = free_string(id_char2);
				id_char = free_string(id_char);
				/* printf("Sent the message = %s to Node:%d\n", message, node->suc); */
				did_work = TCP_send (message, *fd_suc);
				if (did_work == -1) {
					message = free_string(message);
					return -2;
				}
				message = free_string(message);
			}
			else {
				IP_origin_key = free_string(IP_origin_key);
				PORT_origin_key = free_string(PORT_origin_key);
				message = free_string(message);
				return -2;
			}
			if (flag_ret == 1) {
				message = (char *) calloc((128 + 1) , sizeof(char));
				if (message == NULL) {
					return -2;
				}
				flag_ret = 0;
				
				/* Send FND message */
				id_char1 = intToAscii_space(key_to_find);
				if (id_char1 == NULL) {
					message = free_string(message);
					return -2;
				}
				id_char2 = intToAscii_space(seq_aux);
				if (id_char2 == NULL) {
					id_char1 = free_string(id_char1);
					message = free_string(message);
					return -2;
				}
				id_char = intToAscii_space(origin_key);
				if (id_char == NULL) {
					id_char1 = free_string(id_char1);
					id_char2 = free_string(id_char2);
					message = free_string(message);
					return -2;
				}
				message = strcpy(message,"FND");
				message = strcat(message, id_char1);
				message = strcat(message, id_char2);
				message = strcat(message, id_char);
				message = strcat(message, " ");
				message = strcat(message, IP_origin_key);
				message = strcat(message, " ");
				message = strcat(message, PORT_origin_key);
				message = strcat(message, "\n");

				id_char1 = free_string(id_char1);
				id_char2 = free_string(id_char2);
				id_char = free_string(id_char);
				/* printf("Sent the message = %s to Node:%d\n", message, node->suc); */
				did_work = TCP_send (message, *fd_suc);
				if (did_work == -1) {
					message = free_string(message);
					return -2;
				}
				message = free_string(message);
			}
			IP_origin_key = free_string(IP_origin_key);
			PORT_origin_key = free_string(PORT_origin_key);
			message = free_string(message);
			return 0;
			break;
		
		/* RSP: A node delegates to its successor a response destined for the node with
		key k that was started at node i, which will have copied the sequence number n
		from the search that gave rise to the answer */
		case 'R':
			/* Confirm if command is valid */
			if ((strcmp(command_first_word, "RSP") != 0) && (strlen(command_first_word) != 1)) {
				message = free_string(message);
				return -2;
			}
			
			sscanf(command, "%s %d %d %d %s %s", command_first_word, &key_to_find, &seq_aux, &origin_key, string_IP_origin_key, string_PORT_origin_key);

			IP_origin_key = Confirm_IP_port(string_IP_origin_key);
			if (IP_origin_key == NULL) {
				message = free_string(message);
				return -2;
			}
			PORT_origin_key = Confirm_IP_port(string_PORT_origin_key);
			if (PORT_origin_key == NULL) {
				IP_origin_key = free_string(IP_origin_key);
				message = free_string(message);
				return -2;
			}

			/* flag_find = 0 (didn't work)
			flag find = 1 (key already in the node)
			flag_find = 2 (delegate the find to ata UDP)
			flag_find = 3 (delegate the find to suc TCP) */
			flag_find = CloserDistance(node, key_to_find);
			if (flag_find == 1) {
				if (key_to_find == node->key) {
					if (*print == 1) {
						printf("The key you are looking for is in: \n    Node key: %d        | Node IP: %s        | Node PORT: %s \n", origin_key, IP_origin_key, PORT_origin_key);
					}
					else {
                        *IPepred = free_string(*IPepred);
                        *PORTepred = free_string(*PORTepred);
						*key_epred = origin_key;
						*IPepred = (char *) calloc((strlen(IP_origin_key) + 1) , sizeof(char));
						if (*IPepred == NULL) {
							printf("Error, out of memory!\n");
							message = free_string(message);
							IP_origin_key = free_string(IP_origin_key);
                        	PORT_origin_key = free_string(PORT_origin_key);
							return 50;
						}
						strcpy(*IPepred, IP_origin_key);
						*PORTepred = (char *) calloc((strlen(PORT_origin_key) + 1) , sizeof(char));
						if (*PORTepred == NULL) {
							printf("Error, out of memory!\n");
							message = free_string(message);
							IP_origin_key = free_string(IP_origin_key);
                 	       PORT_origin_key = free_string(PORT_origin_key);
							return 50;
						}
						strcpy(*PORTepred, PORT_origin_key);
						message = free_string(message);
						IP_origin_key = free_string(IP_origin_key);
                        PORT_origin_key = free_string(PORT_origin_key);
						return 99;
					}
				}
				else {
					printf("Error: Impossible to find the key");
					IP_origin_key = free_string(IP_origin_key);
					PORT_origin_key = free_string(PORT_origin_key);
					return -2;
				}
			}
			/* Send RSP message */
			else if (flag_find == 2) {
				id_char1 = intToAscii_space(key_to_find);
				if (id_char1 == NULL) {
					message = free_string(message);
					return -2;
				}
				id_char2 = intToAscii_space(seq_aux);
				if (id_char2 == NULL) {
					id_char1 = free_string(id_char1);
					message = free_string(message);
					return -2;
				}
				id_char = intToAscii_space(origin_key);
				if (id_char == NULL) {
					id_char1 = free_string(id_char1);
					id_char2 = free_string(id_char2);
					message = free_string(message);
					return -2;
				}
				message = strcpy(message,"RSP");
				message = strcat(message, id_char1);
				message = strcat(message, id_char2);
				message = strcat(message, id_char);
				message = strcat(message, " ");
				message = strcat(message, IP_origin_key);
				message = strcat(message, " ");
				message = strcat(message, PORT_origin_key);

				id_char1 = free_string(id_char1);
				id_char2 = free_string(id_char2);
				id_char = free_string(id_char);

				udp_error = UDP_sender(node, message, node->IPata, node->PORTata, 2, *addr_udp_epred, fd_pred, 
									  fd_suc, socket_list, size_list, &zero, ACK_hability, saveguard_efnd);
				if (udp_error == -2) {
					if (node->ata != -1) {
						printf("Shortcut to node %d removed\n", node->ata);
						node->ata = -1;
						node->IPata = free_string(node->IPata);
						node->PORTata = free_string(node->PORTata);
					}
                    message = free_string(message);
                    flag_ret = 1;
				}
				else if (udp_error == -1) {
					message = free_string(message);
					return 50;
				}
				else {
					*afd = close_socket(*afd);
					*afd = udp_error;
					IP_origin_key = free_string(IP_origin_key);
					PORT_origin_key = free_string(PORT_origin_key);
					message = free_string(message);
				}
			}
			else if (flag_find == 3) {
				/* Send RSP message */
				id_char1 = intToAscii_space(key_to_find);
				if (id_char1 == NULL) {
					message = free_string(message);
					return -2;
				}
				id_char2 = intToAscii_space(seq_aux);
				if (id_char2 == NULL) {
					id_char1 = free_string(id_char1);
					message = free_string(message);
					return -2;
				}
				id_char = intToAscii_space(origin_key);
				if (id_char == NULL) {
					id_char1 = free_string(id_char1);
					id_char2 = free_string(id_char2);
					message = free_string(message);
					return -2;
				}
				message = strcpy(message,"RSP");
				message = strcat(message, id_char1);
				message = strcat(message, id_char2);
				message = strcat(message, id_char);
				message = strcat(message, " ");
				message = strcat(message, IP_origin_key);
				message = strcat(message, " ");
				message = strcat(message, PORT_origin_key);
				message = strcat(message, "\n");

				id_char1 = free_string(id_char1);
				id_char2 = free_string(id_char2);
				id_char = free_string(id_char);
				/* printf("Sent the message = %s to Node: %d\n", message, node->suc); */
				did_work = TCP_send (message, *fd_suc);
				if (did_work == -1) {
					message = free_string(message);
					IP_origin_key = free_string(IP_origin_key);
					PORT_origin_key = free_string(PORT_origin_key);
					return -2;
				}
				message = free_string(message);
				IP_origin_key = free_string(IP_origin_key);
				PORT_origin_key = free_string(PORT_origin_key);
			}
			else {
				IP_origin_key = free_string(IP_origin_key);
				PORT_origin_key = free_string(PORT_origin_key);
				message = free_string(message);
				return -2;
			}
			if (flag_ret == 1) {
				message = (char *) calloc((128 + 1) , sizeof(char));
				if (message == NULL) {
					return -2;
				}
				flag_ret = 0;
				/* Send RSP message */
				id_char1 = intToAscii_space(key_to_find);
				if (id_char1 == NULL) {
					message = free_string(message);
					return -2;
				}
				id_char2 = intToAscii_space(seq_aux);
				if (id_char2 == NULL) {
					id_char1 = free_string(id_char1);
					message = free_string(message);
					return -2;
				}
				id_char = intToAscii_space(origin_key);
				if (id_char == NULL) {
					id_char1 = free_string(id_char1);
					id_char2 = free_string(id_char2);
					message = free_string(message);
					return -2;
				}
				message = strcpy(message,"RSP");
				message = strcat(message, id_char1);
				message = strcat(message, id_char2);
				message = strcat(message, id_char);
				message = strcat(message, " ");
				message = strcat(message, IP_origin_key);
				message = strcat(message, " ");
				message = strcat(message, PORT_origin_key);
				message = strcat(message, "\n");

				id_char1 = free_string(id_char1);
				id_char2 = free_string(id_char2);
				id_char = free_string(id_char);
				/* printf("Sent the message = %s to Node:%d\n", message, node->suc); */
				did_work = TCP_send (message, *fd_suc);
				if (did_work == -1) {
					message = free_string(message);
					IP_origin_key = free_string(IP_origin_key);
					PORT_origin_key = free_string(PORT_origin_key);
					return -2;
				}
				message = free_string(message);
				IP_origin_key = free_string(IP_origin_key);
				PORT_origin_key = free_string(PORT_origin_key);
			}
			IP_origin_key = free_string(IP_origin_key);
            PORT_origin_key = free_string(PORT_origin_key);
			break;
		
		/* ACK: Confirmation of the previous message */
		case 'A':
			/* Confirm if command is valid */
			if (!strcmp(command_first_word, "ACK ") && (strlen(command_first_word) != 1)) {
				message = free_string(message);
				return -2;
			}
			break;
		
		/* PRED: A node informs its current successor about its new predecessor. */
		case 'P':
			/* Confirm if command is valid */
			if ((strcmp(command_first_word, "PRED") != 0) && (strlen(command_first_word) != 1)) {
				message = free_string(message);
				return -2;
			}

			/* 1st: update predecessor and if is already updated leave
			else send SELF k = node.key command to the predecessor of the atual node */
			if ( (node->pred == -1) && (node->suc == -1) ){
				*fd_suc = close_socket(*fd_suc);
				*fd_pred = close_socket(*fd_pred);
                message = free_string(message);
                return 0;
            }
			sscanf(command, "%s %d %s %s", command_first_word, &aux, string_IPpred, string_PORTpred);
			if (ConfirmKey(aux) == -1) {
				message = free_string(message);
				return -2;
			}

			if (aux == node->pred) {
                message = free_string(message);
				return 0;
			}

			/* Send SELF message */
			id_char = intToAscii_space(node->key);
			if (id_char == NULL) {
				message = free_string(message);
				return -2;
			}
            message = strcpy(message,"SELF");
			message = strcat(message, id_char);
			message = strcat(message, " ");
			message = strcat(message, node->IP);
			message = strcat(message, " ");
			message = strcat(message, node->port);
			message = strcat(message, "\n");

			/* Saveguard node->pred */
			id_char = free_string(id_char);
			salvaguarda_aux = node->pred;
			if (ConfirmKey(aux) == -1) {
				message = free_string(message);
				return -2;
			}

            node->pred = aux;

			/* Saveguard node->IPpred, node->PORTpred */
			if (node->IPpred != NULL) {
				salvaguarda_IP = (char *) calloc((strlen(node->IPpred) + 1) , sizeof(char));
				if (salvaguarda_IP == NULL) {
					printf("Error, out of memory!\n");
					salvaguarda_IP = free_string(salvaguarda_IP);
					salvaguarda_PORT= free_string(salvaguarda_PORT);
					message = free_string(message);
					return 50;
				}
				strcpy(salvaguarda_IP, node->IPpred);
			}
			if (node->PORTpred != NULL) {
				salvaguarda_PORT = (char *) calloc((strlen(node->PORTpred) + 1) , sizeof(char));
				if (salvaguarda_PORT == NULL) {
					printf("Error, out of memory!\n");
					salvaguarda_IP = free_string(salvaguarda_IP);
					salvaguarda_PORT= free_string(salvaguarda_PORT);
					message = free_string(message);
					return 50;
				}
				strcpy(salvaguarda_PORT, node->PORTpred);
			}

			node->IPpred = free_string(node->IPpred);
			node->PORTpred = free_string(node->PORTpred);

			node->IPpred = Confirm_IP_port(string_IPpred);
			if (node->IPpred == NULL) {
				/* Restitution node->pred */
				node->pred = salvaguarda_aux;

				/* Restitution node->IP_pred */
				node->IPpred = free_string(node->IPpred);
				if (salvaguarda_IP != NULL) {
					node->IPpred = (char *) calloc((strlen(salvaguarda_IP) + 1) , sizeof(char));
					if (node->IPpred == NULL) {
						printf("Error, out of memory!\n");
						salvaguarda_IP = free_string(salvaguarda_IP);
						salvaguarda_PORT= free_string(salvaguarda_PORT);
						message = free_string(message);
						return 50;
					}
					strcpy(node->IPpred, salvaguarda_IP);
				}

				message = free_string(message);
				salvaguarda_IP = free_string(salvaguarda_IP);
				salvaguarda_PORT = free_string(salvaguarda_PORT);
				return -2;
			}

			node->PORTpred = Confirm_IP_port(string_PORTpred);
			if (node->PORTpred == NULL) {
				/* Restitution node->pred */
				node->pred = salvaguarda_aux;

				/* Restitution node->IP_pred */
				node->IPpred = free_string(node->IPpred);
				if (salvaguarda_IP != NULL) {
					node->IPpred = (char *) calloc((strlen(salvaguarda_IP) + 1) , sizeof(char));
					if (node->IPpred == NULL) {
						printf("Error, out of memory!\n");
						salvaguarda_IP = free_string(salvaguarda_IP);
						salvaguarda_PORT= free_string(salvaguarda_PORT);
						message = free_string(message);
						return 50;
					}
					strcpy(node->IPpred, salvaguarda_IP);
				}

				/* Restitution node->PORT_pred */
				node->PORTpred = free_string(node->PORTpred);
				if (salvaguarda_PORT != NULL) {
					node->PORTpred = (char *) calloc((strlen(salvaguarda_PORT) + 1) , sizeof(char));
					if (node->PORTpred == NULL) {
						printf("Error, out of memory!\n");
						salvaguarda_IP = free_string(salvaguarda_IP);
						salvaguarda_PORT= free_string(salvaguarda_PORT);
						message = free_string(message);
						return 50;
					}
					strcpy(node->PORTpred, salvaguarda_PORT);
				}

				message = free_string(message);
				salvaguarda_IP = free_string(salvaguarda_IP);
				salvaguarda_PORT = free_string(salvaguarda_PORT);
				return -2;
			}

			if (node->pred != node->suc) {
				if (*fd_suc != *fd_pred) {
					*fd_pred = close_socket(*fd_pred);
				}
				if (node->pred != node->key) {
					*fd_pred = TCP_newfd (node, 0);
				}
			}
			else {
				if (*fd_suc != *fd_pred) {
					*fd_pred = close_socket(*fd_pred);
				}
				*fd_pred = *fd_suc;
			}

			salvaguarda_IP = free_string(salvaguarda_IP);
			salvaguarda_PORT = free_string(salvaguarda_PORT);

			if (node->key == node->pred) {
				salvaguarda_aux = node->suc;
				node->suc = aux;

				/* Saveguard node->IPsuc, node->PORTsuc */
				if (node->IPsuc != NULL) {
					salvaguarda_IP = (char *) calloc((strlen(node->IPsuc) + 1) , sizeof(char));
					if (salvaguarda_IP == NULL) {
						printf("Error, out of memory!\n");
						salvaguarda_IP = free_string(salvaguarda_IP);
						salvaguarda_PORT= free_string(salvaguarda_PORT);
						message = free_string(message);
						return 50;
					}
					strcpy(salvaguarda_IP, node->IPsuc);
				}
				if (node->PORTsuc != NULL) {
					salvaguarda_PORT = (char *) calloc((strlen(node->PORTsuc) + 1) , sizeof(char));
					if (salvaguarda_PORT == NULL) {
						printf("Error, out of memory!\n");
						salvaguarda_IP = free_string(salvaguarda_IP);
						salvaguarda_PORT= free_string(salvaguarda_PORT);
						message = free_string(message);
						return 50;
					}
					strcpy(salvaguarda_PORT, node->PORTsuc);
				}

				node->IPsuc = free_string(node->IPsuc);
				node->PORTsuc =	free_string(node->PORTsuc);

				node->IPsuc = Confirm_IP_port(string_IPpred);
				if (node->IPsuc == NULL) {
					/* Restitution node->suc */
					node->suc = salvaguarda_aux;

					/* Restitution node->IP_suc */
					node->IPsuc = free_string(node->IPsuc);
					if (salvaguarda_IP != NULL) {
						node->IPsuc = (char *) calloc((strlen(salvaguarda_IP) + 1) , sizeof(char));
						if (node->IPsuc == NULL) {
							printf("Error, out of memory!\n");
							salvaguarda_IP = free_string(salvaguarda_IP);
							salvaguarda_PORT= free_string(salvaguarda_PORT);
							message = free_string(message);
							return 50;
						}
						strcpy(node->IPsuc, salvaguarda_IP);
					}

					message = free_string(message);
					salvaguarda_IP = free_string(salvaguarda_IP);
					salvaguarda_PORT = free_string(salvaguarda_PORT);
					return -2;
				}

				node->PORTsuc = Confirm_IP_port(string_PORTpred);
				if (node->PORTsuc == NULL) {
					/* Restitution node->suc */
					node->suc = salvaguarda_aux;

					/* Restitution node->IP_suc */
					node->IPsuc = free_string(node->IPsuc);
					if (salvaguarda_IP != NULL) {
						node->IPsuc = (char *) calloc((strlen(salvaguarda_IP) + 1) , sizeof(char));
						if (node->IPsuc == NULL) {
							printf("Error, out of memory!\n");
							salvaguarda_IP = free_string(salvaguarda_IP);
							salvaguarda_PORT= free_string(salvaguarda_PORT);
							message = free_string(message);
							return 50;
						}
						strcpy(node->IPsuc, salvaguarda_IP);
					}

					/* Restitution node->PORT_suc */
					node->PORTsuc =	free_string(node->PORTsuc);
					if (salvaguarda_PORT != NULL) {
						node->PORTsuc = (char *) calloc((strlen(salvaguarda_PORT) + 1) , sizeof(char));
						if (node->PORTsuc == NULL) {
							printf("Error, out of memory!\n");
							salvaguarda_IP = free_string(salvaguarda_IP);
							salvaguarda_PORT= free_string(salvaguarda_PORT);
							message = free_string(message);
							return 50;
						}
						strcpy(node->PORTsuc, salvaguarda_PORT);
					}

					message = free_string(message);
					salvaguarda_IP = free_string(salvaguarda_IP);
					salvaguarda_PORT = free_string(salvaguarda_PORT);
					return -2;
				}
				if (node->suc != node->pred) {
					if (*fd_suc != *fd_pred) {
						*fd_suc = close_socket(*fd_suc);
					}
					*fd_suc = TCP_newfd (node, 1);
				}
				else {
					*fd_suc = *fd_pred;
				}

				salvaguarda_IP = free_string(salvaguarda_IP);
				salvaguarda_PORT = free_string(salvaguarda_PORT);
			}
			else {
				if (node->pred != -1) {
					/* printf("Sent the message = %s to Node:%d with socket %d\n", message, node->pred, *fd_pred); */
					did_work = TCP_send (message, *fd_pred);
					if (did_work == -1) {
						message = free_string(message);
						return -2;
					}
				}
			}
			id_char = free_string(id_char);
			message = free_string(message);
			break;

		/* SELF: Another node makes itself known to its predecessor */
		case 'S':
			/* 1st: update sucessor and if is already updated leave,
			else send PRED command to the sucessor of the atual node */
			/* Confirm if command is valid */
			if ((strcmp(command_first_word, "SELF") != 0) && (strlen(command_first_word) != 1)) {
				message = free_string(message);
				return -2;
			}
			if (node->suc == -1 && node->pred == -1) {
				message = free_string(message);
				return -2;
			}

			/* Case of invalid pentry */
			if (node->suc == -1 && node->pred != -1) {
				confirm_pentry = 1;
			}

			sscanf(command, "%s %d %s %s", command_first_word, &aux, string_IPsuc, string_PORTsuc);

			if (ConfirmKey(aux) == -1) {
				message = free_string(message);
				return -2;
			}

			if (aux == node->suc) {
                message = free_string(message);
				return 0;
			}

			/* Send PRED message */
			id_char = intToAscii_space(aux);
			auxiliar1 = Confirm_IP_port(string_IPsuc);
			auxiliar2 = Confirm_IP_port(string_PORTsuc);
			if (auxiliar1 == NULL || auxiliar2 == NULL || id_char == NULL) {
				id_char = free_string(id_char);
				auxiliar1 = free_string(auxiliar1);
				auxiliar2 = free_string(auxiliar2);
				message = free_string(message);
				printf("Command not valid\n");
				return -2;
			}
            message = strcpy(message,"PRED");
			message = strcat(message, id_char);
			message = strcat(message, " ");
			message = strcat(message, auxiliar1);
			message = strcat(message, " ");
			message = strcat(message, auxiliar2);
			message = strcat(message, "\n");

			id_char = free_string(id_char);
			auxiliar1 = free_string(auxiliar1);
			auxiliar2 = free_string(auxiliar2);

			if (node->key == node->suc) {
				/* Saveguard node->pred */
				salvaguarda_aux = node->pred;
				node->pred = aux;

				/* Saveguard node->IPpred, node->PORTpred */
				if (node->IPpred != NULL) {
					salvaguarda_IP = (char *) calloc((strlen(node->IPpred) + 1) , sizeof(char));
					if (salvaguarda_IP == NULL) {
						printf("Error, out of memory!\n");
						salvaguarda_IP = free_string(salvaguarda_IP);
						salvaguarda_PORT= free_string(salvaguarda_PORT);
						message = free_string(message);
						return 50;
					}
					strcpy(salvaguarda_IP, node->IPpred);
				}
				if (node->PORTpred != NULL) {
					salvaguarda_PORT = (char *) calloc((strlen(node->PORTpred) + 1) , sizeof(char));
					if (salvaguarda_PORT == NULL) {
						printf("Error, out of memory!\n");
						salvaguarda_IP = free_string(salvaguarda_IP);
						salvaguarda_PORT= free_string(salvaguarda_PORT);
						message = free_string(message);
						return 50;
					}
					strcpy(salvaguarda_PORT, node->PORTpred);
				}

				node->IPpred = free_string(node->IPpred);
				node->PORTpred = free_string(node->PORTpred);

				node->IPpred = Confirm_IP_port(string_IPsuc);
				if (node->IPpred == NULL) {
					/* Restitution node->pred */
					node->pred = salvaguarda_aux;

					/* Restitution node->IP_pred */
					node->IPpred = free_string(node->IPpred);
					if (salvaguarda_IP != NULL) {
						node->IPpred = (char *) calloc((strlen(salvaguarda_IP) + 1) , sizeof(char));
						if (node->IPpred == NULL) {
							printf("Error, out of memory!\n");
							salvaguarda_IP = free_string(salvaguarda_IP);
							salvaguarda_PORT= free_string(salvaguarda_PORT);
							message = free_string(message);
							return 50;
						}
						strcpy(node->IPpred, salvaguarda_IP);
					}

					message = free_string(message);
					salvaguarda_IP = free_string(salvaguarda_IP);
					salvaguarda_PORT = free_string(salvaguarda_PORT);
					return -2;
				}
				node->PORTpred = Confirm_IP_port(string_PORTsuc);
				if (node->PORTpred == NULL) {
					/* Restitution node->pred */
					node->pred = salvaguarda_aux;

					/* Restitution node->IP_pred */
					node->IPpred = free_string(node->IPpred);
					if (salvaguarda_IP != NULL) {
						node->IPpred = (char *) calloc((strlen(salvaguarda_IP) + 1) , sizeof(char));
						if (node->IPpred == NULL) {
							printf("Error, out of memory!\n");
							salvaguarda_IP = free_string(salvaguarda_IP);
							salvaguarda_PORT= free_string(salvaguarda_PORT);
							message = free_string(message);
							return 50;
						}
						strcpy(node->IPpred, salvaguarda_IP);
					}

					/* Restitution node->PORT_pred */
					node->PORTpred = free_string(node->PORTpred);
					if (salvaguarda_PORT != NULL) {
						node->PORTpred = (char *) calloc((strlen(salvaguarda_PORT) + 1) , sizeof(char));
						if (node->PORTpred == NULL) {
							printf("Error, out of memory!\n");
							salvaguarda_IP = free_string(salvaguarda_IP);
							salvaguarda_PORT= free_string(salvaguarda_PORT);
							message = free_string(message);
							return 50;
						}
						strcpy(node->PORTpred, salvaguarda_PORT);
					}

					message = free_string(message);
					salvaguarda_IP = free_string(salvaguarda_IP);
					salvaguarda_PORT = free_string(salvaguarda_PORT);
					return -2;
				}
				*fd_suc = close_socket(*fd_suc);
				*fd_pred = close_socket(*fd_pred);
				*fd_pred = TCP_newfd(node, 0);
				*fd_suc = *fd_pred;

				salvaguarda_IP = free_string(salvaguarda_IP);
				salvaguarda_PORT = free_string(salvaguarda_PORT);

				/* As the next step is skipped, we need to send next SELF mesage */
				id_char = intToAscii_space(node->key);
				if (id_char == NULL) {
					message = free_string(message);
					return -2;
				}
				message = strcpy(message,"SELF");
				message = strcat(message, id_char);
				message = strcat(message, " ");
				message = strcat(message, node->IP);
				message = strcat(message, " ");
				message = strcat(message, node->port);
				message = strcat(message, "\n");

				id_char = free_string(id_char);
				node->IPsuc = free_string(node->IPsuc);
				node->PORTsuc =	free_string(node->PORTsuc);

				salvaguarda_aux = node->suc;
				node->suc = aux;

				/* Saveguard node->IPsuc, node->PORTsuc */
				if (node->IPsuc != NULL) {
					salvaguarda_IP = (char *) calloc((strlen(node->IPsuc) + 1) , sizeof(char));
					if (salvaguarda_IP == NULL) {
						printf("Error, out of memory!\n");
						salvaguarda_IP = free_string(salvaguarda_IP);
						salvaguarda_PORT= free_string(salvaguarda_PORT);
						message = free_string(message);
						return 50;
					}
					strcpy(salvaguarda_IP, node->IPsuc);
				}
				if (node->PORTsuc != NULL) {
					salvaguarda_PORT = (char *) calloc((strlen(node->PORTsuc) + 1) , sizeof(char));
					if (salvaguarda_PORT == NULL) {
						printf("Error, out of memory!\n");
						salvaguarda_IP = free_string(salvaguarda_IP);
						salvaguarda_PORT= free_string(salvaguarda_PORT);
						message = free_string(message);
						return 50;
					}
					strcpy(salvaguarda_PORT, node->PORTsuc);
				}

				node->IPsuc = free_string(node->IPsuc);
				node->PORTsuc =	free_string(node->PORTsuc);

				node->IPsuc = Confirm_IP_port(string_IPsuc);
				if (node->IPsuc == NULL) {
					/* Restitution node->suc */
					node->suc = salvaguarda_aux;

					/* Restitution node->IP_suc */
					node->IPsuc = free_string(node->IPsuc);
					if (salvaguarda_IP != NULL) {
						node->IPsuc = (char *) calloc((strlen(salvaguarda_IP) + 1) , sizeof(char));
						if (node->IPsuc == NULL) {
							printf("Error, out of memory!\n");
							salvaguarda_IP = free_string(salvaguarda_IP);
							salvaguarda_PORT= free_string(salvaguarda_PORT);
							message = free_string(message);
							return 50;
						}
						strcpy(node->IPsuc, salvaguarda_IP);
					}

					message = free_string(message);
					salvaguarda_IP = free_string(salvaguarda_IP);
					salvaguarda_PORT = free_string(salvaguarda_PORT);
					return -2;
				}

				node->PORTsuc = Confirm_IP_port(string_PORTsuc);
				if (node->PORTsuc == NULL) {
					/* Restitution node->suc */
					node->suc = salvaguarda_aux;

					/* Restitution node->IP_suc */
					node->IPsuc = free_string(node->IPsuc);
					if (salvaguarda_IP != NULL) {
						node->IPsuc = (char *) calloc((strlen(salvaguarda_IP) + 1) , sizeof(char));
						if (node->IPsuc == NULL) {
							printf("Error, out of memory!\n");
							salvaguarda_IP = free_string(salvaguarda_IP);
							salvaguarda_PORT= free_string(salvaguarda_PORT);
							message = free_string(message);
							return 50;
						}
						strcpy(node->IPsuc, salvaguarda_IP);
					}

					/* Restitution node->PORT_suc */
					node->PORTsuc =	free_string(node->PORTsuc);
					if (salvaguarda_PORT != NULL) {
						node->PORTsuc = (char *) calloc((strlen(salvaguarda_PORT) + 1) , sizeof(char));
						if (node->PORTsuc == NULL) {
							printf("Error, out of memory!\n");
							salvaguarda_IP = free_string(salvaguarda_IP);
							salvaguarda_PORT= free_string(salvaguarda_PORT);
							message = free_string(message);
							return 50;
						}
						strcpy(node->PORTsuc, salvaguarda_PORT);
					}

					message = free_string(message);
					salvaguarda_IP = free_string(salvaguarda_IP);
					salvaguarda_PORT = free_string(salvaguarda_PORT);
					return -2;
				}
				if (node->suc != node->pred) {
					if (*fd_suc != *fd_pred) {
						*fd_suc = close_socket(*fd_suc);
					}
					*fd_suc = TCP_newfd (node, 1);
				}
				else {
					*fd_suc = *fd_pred;
				}

				salvaguarda_IP = free_string(salvaguarda_IP);
				salvaguarda_PORT = free_string(salvaguarda_PORT);

				if (node->suc != -1) {
					/* printf("Sent the message = %s Socket used = %d\n", message, *fd_suc); */
					did_work = TCP_send (message, *fd_suc);
					if (did_work == -1) {
						message = free_string(message);
						return -2;
					}
				}
				if (confirm_pentry == 1) {
					if (node->pred != node->suc && distance(node->pred, node->key) > distance(node->pred, node->suc)) {
						leave(node, message, fd_suc, fd_pred, socket_list, size_list);
					}
					message = free_string(message);
					return 0;
				}
			}
			else {
				if (node->suc != -1) {
					/* printf("Sent the message = %s to Node:%d with socket %d\n", message, node->suc, *fd_suc); */
					did_work = TCP_send (message, *fd_suc);
					if (did_work == -1) {
						message = free_string(message);

						node->suc = aux;
						/* Saveguard node->IPsuc, node->PORTsuc */
						if (node->IPsuc != NULL) {
							salvaguarda_IP = (char *) calloc((strlen(node->IPsuc) + 1) , sizeof(char));
							if (salvaguarda_IP == NULL) {
								printf("Error, out of memory!\n");
								salvaguarda_IP = free_string(salvaguarda_IP);
								salvaguarda_PORT= free_string(salvaguarda_PORT);
								message = free_string(message);
								return 50;
							}
							strcpy(salvaguarda_IP, node->IPsuc);
						}
						if (node->PORTsuc != NULL) {
							salvaguarda_PORT = (char *) calloc((strlen(node->PORTsuc) + 1) , sizeof(char));
							if (salvaguarda_PORT == NULL) {
								printf("Error, out of memory!\n");
								salvaguarda_IP = free_string(salvaguarda_IP);
								salvaguarda_PORT= free_string(salvaguarda_PORT);
								message = free_string(message);
								return 50;
							}
							strcpy(salvaguarda_PORT, node->PORTsuc);
						}

						node->IPsuc = free_string(node->IPsuc);
						node->PORTsuc =	free_string(node->PORTsuc);

						node->IPsuc = Confirm_IP_port(string_IPsuc);
						if (node->IPsuc == NULL) {
							/* Restitution node->IP_suc */
							node->IPsuc = free_string(node->IPsuc);
							if (salvaguarda_IP != NULL) {
								node->IPsuc = (char *) calloc((strlen(salvaguarda_IP) + 1) , sizeof(char));
								if (node->IPsuc == NULL) {
									printf("Error, out of memory!\n");
									salvaguarda_IP = free_string(salvaguarda_IP);
									salvaguarda_PORT= free_string(salvaguarda_PORT);
									message = free_string(message);
									return 50;
								}
								strcpy(node->IPsuc, salvaguarda_IP);
							}

							message = free_string(message);
							salvaguarda_IP = free_string(salvaguarda_IP);
							salvaguarda_PORT = free_string(salvaguarda_PORT);
							return -2;
						}

						node->PORTsuc = Confirm_IP_port(string_PORTsuc);
						if (node->PORTsuc == NULL) {
							/* Restitution node->IP_suc */
							node->IPsuc = free_string(node->IPsuc);
							if (salvaguarda_IP != NULL) {
								node->IPsuc = (char *) calloc((strlen(salvaguarda_IP) + 1) , sizeof(char));
								if (node->IPsuc == NULL) {
									printf("Error, out of memory!\n");
									salvaguarda_IP = free_string(salvaguarda_IP);
									salvaguarda_PORT= free_string(salvaguarda_PORT);
									message = free_string(message);
									return 50;
								}
								strcpy(node->IPsuc, salvaguarda_IP);
							}

							/* Restitution node->PORT_suc */
							node->PORTsuc =	free_string(node->PORTsuc);
							if (salvaguarda_PORT != NULL) {
								node->PORTsuc = (char *) calloc((strlen(salvaguarda_PORT) + 1) , sizeof(char));
								if (node->PORTsuc == NULL) {
									printf("Error, out of memory!\n");
									salvaguarda_IP = free_string(salvaguarda_IP);
									salvaguarda_PORT= free_string(salvaguarda_PORT);
									message = free_string(message);
									return 50;
								}
								strcpy(node->PORTsuc, salvaguarda_PORT);
							}

							message = free_string(message);
							salvaguarda_IP = free_string(salvaguarda_IP);
							salvaguarda_PORT = free_string(salvaguarda_PORT);
							return -2;
						}

						if (node->suc != node->pred) {
							if (*fd_suc != *fd_pred) {
								*fd_suc = close_socket(*fd_suc);
							}
							*fd_suc = TCP_newfd (node, 1);
						}
						else {
							*fd_suc = *fd_pred;
						}

						salvaguarda_IP = free_string(salvaguarda_IP);
						salvaguarda_PORT = free_string(salvaguarda_PORT);

						if (confirm_pentry == 1) {
							if (node->pred != node->suc && distance(node->pred, node->key) > distance(node->pred, node->suc)) {
								leave(node, message, fd_suc, fd_pred, socket_list, size_list);
							}
							message = free_string(message);
							return 0;
						}

						return 0;
					}
				}
				node->IPsuc = free_string(node->IPsuc);
				node->PORTsuc =	free_string(node->PORTsuc);

				salvaguarda_aux = node->suc;
				node->suc = aux;

				/* Saveguard node->IPsuc, node->PORTsuc */
				if (node->IPsuc != NULL) {
					salvaguarda_IP = (char *) calloc((strlen(node->IPsuc) + 1) , sizeof(char));
					if (salvaguarda_IP == NULL) {
						printf("Error, out of memory!\n");
						salvaguarda_IP = free_string(salvaguarda_IP);
						salvaguarda_PORT= free_string(salvaguarda_PORT);
						message = free_string(message);
						return 50;
					}
					strcpy(salvaguarda_IP, node->IPsuc);
				}
				if (node->PORTsuc != NULL) {
					salvaguarda_PORT = (char *) calloc((strlen(node->PORTsuc) + 1) , sizeof(char));
					if (salvaguarda_PORT == NULL) {
						printf("Error, out of memory!\n");
						salvaguarda_IP = free_string(salvaguarda_IP);
						salvaguarda_PORT= free_string(salvaguarda_PORT);
						message = free_string(message);
						return 50;
					}
					strcpy(salvaguarda_PORT, node->PORTsuc);
				}

				node->IPsuc = free_string(node->IPsuc);
				node->PORTsuc =	free_string(node->PORTsuc);

				node->IPsuc = Confirm_IP_port(string_IPsuc);
				if (node->IPsuc == NULL) {
					/* Restitution node->suc */
					node->suc = salvaguarda_aux;

					/* Restitution node->IP_suc */
					node->IPsuc = free_string(node->IPsuc);
					if (salvaguarda_IP != NULL) {
						node->IPsuc = (char *) calloc((strlen(salvaguarda_IP) + 1) , sizeof(char));
						if (node->IPsuc == NULL) {
							printf("Error, out of memory!\n");
							salvaguarda_IP = free_string(salvaguarda_IP);
							salvaguarda_PORT= free_string(salvaguarda_PORT);
							message = free_string(message);
							return 50;
						}
						strcpy(node->IPsuc, salvaguarda_IP);
					}

					message = free_string(message);
					salvaguarda_IP = free_string(salvaguarda_IP);
					salvaguarda_PORT = free_string(salvaguarda_PORT);
					return -2;
				}
				node->PORTsuc = Confirm_IP_port(string_PORTsuc);
				if (node->PORTsuc == NULL) {
					/* Restitution node->suc */
					node->suc = salvaguarda_aux;

					/* Restitution node->IP_suc */
					node->IPsuc = free_string(node->IPsuc);
					if (salvaguarda_IP != NULL) {
						node->IPsuc = (char *) calloc((strlen(salvaguarda_IP) + 1) , sizeof(char));
						if (node->IPsuc == NULL) {
							printf("Error, out of memory!\n");
							salvaguarda_IP = free_string(salvaguarda_IP);
							salvaguarda_PORT= free_string(salvaguarda_PORT);
							message = free_string(message);
							return 50;
						}
						strcpy(node->IPsuc, salvaguarda_IP);
					}

					/* Restitution node->PORT_suc */
					node->PORTsuc =	free_string(node->PORTsuc);
					if (salvaguarda_PORT != NULL) {
						node->PORTsuc = (char *) calloc((strlen(salvaguarda_PORT) + 1) , sizeof(char));
						if (node->PORTsuc == NULL) {
							printf("Error, out of memory!\n");
							salvaguarda_IP = free_string(salvaguarda_IP);
							salvaguarda_PORT= free_string(salvaguarda_PORT);
							message = free_string(message);
							return 50;
						}
						strcpy(node->PORTsuc, salvaguarda_PORT);
					}

					message = free_string(message);
					salvaguarda_IP = free_string(salvaguarda_IP);
					salvaguarda_PORT = free_string(salvaguarda_PORT);
					return -2;
				}

				if (node->suc != node->pred) {
					if (*fd_suc != *fd_pred) {
						*fd_suc = close_socket(*fd_suc);
					}
					*fd_suc = TCP_newfd (node, 1);
				}
				else {
					if (*fd_suc != *fd_pred) {
						*fd_suc = close_socket(*fd_suc);
					}
					*fd_suc = *fd_pred;
				}

				salvaguarda_IP = free_string(salvaguarda_IP);
				salvaguarda_PORT = free_string(salvaguarda_PORT);
			}
			if (confirm_pentry == 1) {
				if (node->pred != node->suc && distance(node->pred, node->key) > distance(node->pred, node->suc)) {
					leave(node, message, fd_suc, fd_pred, socket_list, size_list);
				}
				message = free_string(message);
				return 0;
			}
			message = free_string(message);
			break;
		case 'E':
			switch(command_first_word[1]) {
				/* EFND: An incoming node asks a node in the ring to find out its position in that ring, returning
				the key, IP address and port of its future predecessor. */
				case 'F':
					/* Confirm if command is valid */
					if ((strcmp(command_first_word, "EFND") != 0) && (strlen(command_first_word) != 1)) {
						message = free_string(message);
						return -2;
					}
					*saveguard_efnd = 1;
					*print = 0;
					sscanf(command, "%s %d", command_first_word, &(aux));

					/* flag_find = 0 (didn't work)
					flag find = 1 (key already in the node)
					flag_find = 2 (delegate the find to ata UDP)
					flag_find = 3 (delegate the find to suc TCP) */
					if (ConfirmKey(aux) == -1) {
						message = free_string(message);
						return -2;
					}
					flag_find = CloserDistance(node, aux);
					if (flag_find == 1) {
						if (*print == 1) {
							printf("key %d is in: \n    Node key: %d        | Node IP: %s        | Node PORT: %s \n", aux, node->key, node->IP, node->port);
						}
						else {
                            *IPepred = free_string(*IPepred);
                            *PORTepred = free_string(*PORTepred);
							*key_epred = node->key;
							*IPepred = (char *) calloc((strlen(node->IP) + 1) , sizeof(char));
							if (*IPepred == NULL) {
								printf("Error, out of memory!\n");
								message = free_string(message);
								return 50;
							}
							strcpy(*IPepred, node->IP);
							*PORTepred = (char *) calloc((strlen(node->port) + 1) , sizeof(char));
							if (*PORTepred == NULL) {
								printf("Error, out of memory!\n");
								message = free_string(message);
								return 50;
							}
							strcpy(*PORTepred, node->port);
							message = free_string(message);
							return 99;
						}
					}
					/* Send FND message */
					else if (flag_find == 2) {
						id_char1 = intToAscii_space(aux);
						if (id_char1 == NULL) {
							message = free_string(message);
							return -2;
						}
						id_char2 = intToAscii_space(*sequence_num);
						if (id_char2 == NULL) {
							id_char1 = free_string(id_char1);
							message = free_string(message);
							return -2;
						}
						(*sequence_num)++;
						id_char = intToAscii_space(node->key);
						if (id_char == NULL) {
							id_char1 = free_string(id_char1);
							id_char2 = free_string(id_char2);
							message = free_string(message);
							return -2;
						}
						message = strcpy(message,"FND");
						message = strcat(message, id_char1);
						message = strcat(message, id_char2);
						message = strcat(message, id_char);
						message = strcat(message, " ");
						message = strcat(message, node->IP);
						message = strcat(message, " ");
						message = strcat(message, node->port);

						id_char1 = free_string(id_char1);
						id_char2 = free_string(id_char2);
						id_char = free_string(id_char);

						udp_error = UDP_sender(node, message, node->IPata, node->PORTata, 2, *addr_udp_epred, 
											  fd_pred, fd_suc, socket_list, size_list, &zero, ACK_hability, saveguard_efnd);
						if (udp_error == -2) {
							if (node->ata != -1) {
								printf("Shortcut to node %d removed\n", node->ata);
								node->ata = -1;
								node->IPata = free_string(node->IPata);
								node->PORTata = free_string(node->PORTata);
							}
							message = free_string(message);
							flag_ret = 1;
						}
						else if (udp_error == -1) {
							message = free_string(message);
							return 50;
						}
						else {
							*afd = close_socket(*afd);
							*afd = udp_error;
							message = free_string(message);
						}
					}
					else if (flag_find == 3) {
						/* Send FND message */
						id_char1 = intToAscii_space(aux);
						if (id_char1 == NULL) {
							message = free_string(message);
							return -2;
						}
						id_char2 = intToAscii_space(*sequence_num);
						if (id_char2 == NULL) {
							id_char1 = free_string(id_char1);
							message = free_string(message);
							return -2;
						}
						(*sequence_num)++;
						id_char = intToAscii_space(node->key);
						if (id_char == NULL) {
							id_char1 = free_string(id_char1);
							id_char2 = free_string(id_char2);
							message = free_string(message);
							return -2;
						}
						message = strcpy(message,"FND");
						message = strcat(message, id_char1);
						message = strcat(message, id_char2);
						message = strcat(message, id_char);
						message = strcat(message, " ");
						message = strcat(message, node->IP);
						message = strcat(message, " ");
						message = strcat(message, node->port);
						message = strcat(message, "\n");

						id_char1 = free_string(id_char1);
						id_char2 = free_string(id_char2);
						id_char = free_string(id_char);
						/* printf("Sent the message = %s to Node:%d\n", message, node->suc); */
						did_work = TCP_send (message, *fd_suc);
						if (did_work == -1) {
							message = free_string(message);
							return -2;
						}
						message = free_string(message);
					}
					else {
						message = free_string(message);
						return -2;
					}
					if (flag_ret == 1) {
						message = (char *) calloc((128 + 1) , sizeof(char));
						if (message == NULL) {
							return -2;
						}
						flag_ret = 0;
						/* Send FND message */
						id_char1 = intToAscii_space(aux);
						if (id_char1 == NULL) {
							message = free_string(message);
							return -2;
						}
						id_char2 = intToAscii_space(*sequence_num);
						if (id_char2 == NULL) {
							id_char1 = free_string(id_char1);
							message = free_string(message);
							return -2;
						}
						(*sequence_num)++;
						id_char = intToAscii_space(node->key);
						if (id_char == NULL) {
							id_char1 = free_string(id_char1);
							id_char2 = free_string(id_char2);
							message = free_string(message);
							return -2;
						}
						message = strcpy(message,"FND");
						message = strcat(message, id_char1);
						message = strcat(message, id_char2);
						message = strcat(message, id_char);
						message = strcat(message, " ");
						message = strcat(message, node->IP);
						message = strcat(message, " ");
						message = strcat(message, node->port);
						message = strcat(message, "\n");

						id_char1 = free_string(id_char1);
						id_char2 = free_string(id_char2);
						id_char = free_string(id_char);
						/* printf("Sent the message = %s to Node:%d\n", message, node->suc); */
						did_work = TCP_send (message, *fd_suc);
						if (did_work == -1) {
							message = free_string(message);
							return -2;
						}
						message = free_string(message);
					}

					message = free_string(message);
					break;

				/* EPRED: A ring node responds to an incoming node with the key of its future predecessor */
				case 'P':
					/* Confirm if command is valid */
					if ((strcmp(command_first_word, "EPRED") != 0) && (strlen(command_first_word) != 1)) {
						message = free_string(message);
						return -2;
					}
					sscanf(command, "%s %d %s %s", command_first_word, &(aux), string_IPpred, string_PORTpred);
					if (node->suc != -1) {
						if (aux >= node->suc || aux <= node->key) {
							printf("Pentry/Bentry not valid\n");
							message = free_string(message);
							return -2;
						}
					}

					/* Saveguard node->pred */
					salvaguarda_aux = node->pred;
					if (ConfirmKey(aux) == -1) {
						message = free_string(message);
						return -2;
					}
					node->pred = aux;

					/* Saveguard node->IPpred, node->PORTpred */
					if (node->IPpred != NULL) {
						salvaguarda_IP = (char *) calloc((strlen(node->IPpred) + 1) , sizeof(char));
						if (salvaguarda_IP == NULL) {
							printf("Error, out of memory!\n");
							salvaguarda_IP = free_string(salvaguarda_IP);
							salvaguarda_PORT= free_string(salvaguarda_PORT);
							message = free_string(message);
							return 50;
						}
						strcpy(salvaguarda_IP, node->IPpred);
					}
					if (node->PORTpred != NULL) {
						salvaguarda_PORT = (char *) calloc((strlen(node->PORTpred) + 1) , sizeof(char));
						if (salvaguarda_PORT == NULL) {
							printf("Error, out of memory!\n");
							salvaguarda_IP = free_string(salvaguarda_IP);
							salvaguarda_PORT= free_string(salvaguarda_PORT);
							message = free_string(message);
							return 50;
						}
						strcpy(salvaguarda_PORT, node->PORTpred);
					}

					node->IPpred = free_string(node->IPpred);
					node->PORTpred = free_string(node->PORTpred);

					node->IPpred = Confirm_IP_port(string_IPpred);
					if (node->IPpred == NULL) {
						/* Restitution node->pred */
						node->pred = salvaguarda_aux;

						/* Restitution node->IP_pred */
						node->IPpred = free_string(node->IPpred);
						if (salvaguarda_IP != NULL) {
							node->IPpred = (char *) calloc((strlen(salvaguarda_IP) + 1) , sizeof(char));
							if (node->IPpred == NULL) {
								printf("Error, out of memory!\n");
								salvaguarda_IP = free_string(salvaguarda_IP);
								salvaguarda_PORT= free_string(salvaguarda_PORT);
								message = free_string(message);
								return 50;
							}
							strcpy(node->IPpred, salvaguarda_IP);
						}

						message = free_string(message);
						salvaguarda_IP = free_string(salvaguarda_IP);
						salvaguarda_PORT = free_string(salvaguarda_PORT);
						return -2;
					}

					node->PORTpred = Confirm_IP_port(string_PORTpred);
					if (node->PORTpred == NULL) {
						/* Restitution node->pred */
						node->pred = salvaguarda_aux;

						/* Restitution node->IP_pred */
						node->IPpred = free_string(node->IPpred);
						if (salvaguarda_IP != NULL) {
							node->IPpred = (char *) calloc((strlen(salvaguarda_IP) + 1) , sizeof(char));
							if (node->IPpred == NULL) {
								printf("Error, out of memory!\n");
								salvaguarda_IP = free_string(salvaguarda_IP);
								salvaguarda_PORT= free_string(salvaguarda_PORT);
								message = free_string(message);
								return 50;
							}
							strcpy(node->IPpred, salvaguarda_IP);
						}

						/* Restitution node->PORT_pred */
						node->PORTpred = free_string(node->PORTpred);
						if (salvaguarda_PORT != NULL) {
							node->PORTpred = (char *) calloc((strlen(salvaguarda_PORT) + 1) , sizeof(char));
							if (node->PORTpred == NULL) {
								printf("Error, out of memory!\n");
								salvaguarda_IP = free_string(salvaguarda_IP);
								salvaguarda_PORT= free_string(salvaguarda_PORT);
								message = free_string(message);
								return 50;
							}
							strcpy(node->PORTpred, salvaguarda_PORT);
						}

						message = free_string(message);
						salvaguarda_IP = free_string(salvaguarda_IP);
						salvaguarda_PORT = free_string(salvaguarda_PORT);
						return -2;
					}

					if (node->pred != node->suc) {
						if (*fd_suc != *fd_pred) {
							*fd_pred = close_socket(*fd_pred);
						}
						*fd_pred = TCP_newfd (node, 0);
					}
					else {
						if (*fd_suc != *fd_pred) {
							*fd_pred = close_socket(*fd_pred);
						}
						*fd_pred = *fd_suc;
					}

					salvaguarda_IP = free_string(salvaguarda_IP);
					salvaguarda_PORT = free_string(salvaguarda_PORT);

					/* Send SELF message */
					id_char = intToAscii_space(node->key);
					if (id_char == NULL) {
						message = free_string(message);
						return -2;
					}
					message = strcpy(message,"SELF");
					message = strcat(message, id_char);
					message = strcat(message, " ");
					message = strcat(message, node->IP);
					message = strcat(message, " ");
					message = strcat(message, node->port);
					message = strcat(message, "\n");
					/* if you send pentry to yourself, that will be no effect */
					if (node->key != node->pred) {
						/* printf("Sent the message = %s to Node:%d\n", message, node->pred); */
						did_work = TCP_send (message, *fd_pred);
						if (did_work == -1) {
							message = free_string(message);
							return -2;
						}
					}
					id_char = free_string(id_char);
					break;
			}
			break;
		default:
		    printf("Command not found \n");
		    message = free_string(message);
			return -2;
	}
	message = free_string(message);
	return 0;
}
