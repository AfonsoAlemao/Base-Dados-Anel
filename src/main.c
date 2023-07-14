/**********************************************************************************
* (c) Rui Daniel 96317, Afonso Alemao 96135
* Last Modified: 11/04/2022
*
* Name:
*   main.c
*
* Description:
*   Main for the RCI IST 2022 2º Semester Project: Database Ring with Chords
*
* Comments:
*
**********************************************************************************/

#include "aux.h"
#include "tcp.h"
#include "udp.h"
#include "commands.h"

/**********************************************************************************
 * main()
 *
 * Arguments: argc - number of received arguments
 * 			  argv - pointer to a array of the arguments
 *
 * Return: int (0)
 * Side effects: ends the program in the end
 *
 * Description: Executes the select function to extract the command to process
 *
 ********************************************************************************/

int main(int argc, char *argv[]) {

	/* We need 4 arguments */
    if (argc != 4) {
        printf("You need 4 arguments\n");
        return 0;
    }

    Nodes node;
	Nodes *ptrnode = &node;
	node = Inicialize_Node(node);

	struct addrinfo hints_tcp, *res_tcp = NULL, *res_udp = NULL, hints_udp;
	int fd_tcp = 0, fd_udp = 0, newfd, errcode = 0, flag = -1, fd_suc = 0, fd_pred = 0, fd_epred2 = 0;
	int readcode = 0, sequence_num = 0, print = 1, afd = 0, nbytes = 0, i = 0, ack_hability = 0, saveguard_efnd = 0;
	ssize_t nread;
	fd_set rfds;
	enum {idle, busy} state;
	int maxfd, counter, end = 1, max_list = 0, flag_select = 1, timeout = 0;
	struct sockaddr addr_tcp, addr_udp, addr_udp_epred, addr_epred2;
	socklen_t addrlen_udp, addrlen_tcp, addrlen_epred2;
	int *socket_list = NULL, size_list = 200;
	socket_list = (int *) calloc(size_list, sizeof(int));
	if (socket_list == NULL) {
		return 0;
	}

	char buffer[128 + 1];
	char *id_char = NULL;
	int key_epred = 0, nleft = 0;
	char *IPepred = NULL;
	char *PORTepred = NULL;
	char *message = NULL, *ptrr = NULL;
    message = (char *) calloc((128 + 1) , sizeof(char));
	if (message == NULL) {
		return 0;
	}

	/* Protect against SIGPIPE */
	struct sigaction act;
	memset(&act, 0, sizeof act);
	act.sa_handler = SIG_IGN;
	if (sigaction(SIGPIPE, &act, NULL) == -1) {
		Closes_Frees(fd_tcp, fd_udp, afd, &node, res_tcp, res_udp, message, IPepred, PORTepred, fd_suc, fd_pred, socket_list, size_list);
        fprintf(stderr, "error: SIGPIPE: %s\n", strerror(errno));
        return 0;
    }

	/* Show the interface to the user */
	interface();

	/* Input arguments confirmation */
    node.key = ConfirmKey(atoi(argv[1]));
    if (node.key == -1) {
		Closes_Frees(fd_tcp, fd_udp, afd, &node, res_tcp, res_udp, message, IPepred, PORTepred, fd_suc, fd_pred, socket_list, size_list);
		fprintf(stderr, "error: key not valid\n");
		return 0;
	}
	node.IP = Confirm_IP_port(argv[2]);
	if (node.IP == NULL) {
		Closes_Frees(fd_tcp, fd_udp, afd, &node, res_tcp, res_udp, message, IPepred, PORTepred, fd_suc, fd_pred, socket_list, size_list);
		fprintf(stderr, "error: IP not valid\n");
		return 0;
	}
	node.port = Confirm_IP_port(argv[3]);
	if (node.port == NULL) {
		Closes_Frees(fd_tcp, fd_udp, afd, &node, res_tcp, res_udp, message, IPepred, PORTepred, fd_suc, fd_pred, socket_list, size_list);
		fprintf(stderr, "error: port not valid\n");
		return 0;
	}

	/* Create a tcp socket */
	if ((fd_tcp = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		Closes_Frees(fd_tcp, fd_udp, afd, &node, res_tcp, res_udp, message, IPepred, PORTepred, fd_suc, fd_pred, socket_list, size_list);
        fprintf(stderr, "error: %s\n", strerror(errno));
        return 0;
    }

	memset(&hints_tcp, 0, sizeof hints_tcp);
	memset(&addr_tcp, 0, sizeof(addr_tcp));
	hints_tcp.ai_family = AF_INET; //IPv4
	hints_tcp.ai_socktype = SOCK_STREAM; //TCP socket
	hints_tcp.ai_flags = AI_PASSIVE;

	if ((errcode = getaddrinfo(node.IP, node.port, &hints_tcp, &res_tcp)) != 0) {
        fprintf(stderr, "error: getaddrinfo: %s\n", gai_strerror(errcode));
		Closes_Frees(fd_tcp, fd_udp, afd, &node, res_tcp, res_udp, message, IPepred, PORTepred, fd_suc, fd_pred, socket_list, size_list);
		return 0;
    }

	/* Register the server well known address (and port) with the system */
	if (bind(fd_tcp, res_tcp->ai_addr, res_tcp->ai_addrlen) == -1) {
		Closes_Frees(fd_tcp, fd_udp, afd, &node, res_tcp, res_udp, message, IPepred, PORTepred, fd_suc, fd_pred, socket_list, size_list);
        fprintf(stderr, "error: bind: %s\n", strerror(errno));
        return 0;
    }

	/* Instruct the kernel to accept incoming connection requests for this socket. The 
	backlog argument defines the maximum length the queue of pending connections may grow to. */
	if (listen(fd_tcp, 5) == -1) {
		Closes_Frees(fd_tcp, fd_udp, afd, &node, res_tcp, res_udp, message, IPepred, PORTepred, fd_suc, fd_pred, socket_list, size_list);
        fprintf(stderr, "error: listen: %s\n", strerror(errno));
        return 0;
    }

	/* Create UDP socket */
    if ((fd_udp = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		Closes_Frees(fd_tcp, fd_udp, afd, &node, res_tcp, res_udp, message, IPepred, PORTepred, fd_suc, fd_pred, socket_list, size_list);
        fprintf(stderr, "error: %s\n", strerror(errno));
        return 0;
    }
	memset(&hints_udp, 0, sizeof hints_udp);
	memset(&addr_udp, 0, sizeof(addr_udp));
	hints_udp.ai_family = AF_INET; //IPv4
	hints_udp.ai_socktype = SOCK_DGRAM; //UDP socket
	hints_udp.ai_flags = AI_PASSIVE;

	if ((errcode = getaddrinfo(node.IP, node.port, &hints_udp, &res_udp)) != 0) {
        fprintf(stderr, "error: getaddrinfo: %s\n", gai_strerror(errcode));
		Closes_Frees(fd_tcp, fd_udp, afd, &node, res_tcp, res_udp, message, IPepred, PORTepred, fd_suc, fd_pred, socket_list, size_list);
		return 0;
    }
	if (bind(fd_udp, res_udp->ai_addr, res_udp->ai_addrlen) == -1) {
		Closes_Frees(fd_tcp, fd_udp, afd, &node, res_tcp, res_udp, message, IPepred, PORTepred, fd_suc, fd_pred, socket_list, size_list);
        fprintf(stderr, "error: bind: %s\n", strerror(errno));
        return 0;
    }

	/*	rfds: set of pointers to files that are received.
	afds: socket already accepted and it's the next to be read.
	If exist an afd, fd saves the next in line.
	Else we are idle: fd will be accepted and saved in afd.
	the programm only get blocked (waiting) in select.

	accept: extract the first connection request on
	the queue of pending connections. Returns a socket
	associated with the new connection. */

	state = idle;
	while(end) {
		CheckShortcut (ptrnode);
		ack_hability = CheckACKhability(ptrnode);
		FD_ZERO(&rfds);
		switch (state) {
			case idle:
				FD_SET(fd_tcp, &rfds);
				FD_SET(fd_udp, &rfds);
				FD_SET(0, &rfds);
				max_list = 0;
				for (i = 0; i < size_list; i++) {
					if (socket_list[i] != 0) {
						if (socket_list[i] > max_list) {
							max_list = socket_list[i];
						}
						FD_SET(socket_list[i], &rfds);
					}
				}
				maxfd = max(max(max(fd_tcp, fd_udp), 0), max_list);
				break;
			case busy:
				FD_SET(fd_tcp, &rfds);
				FD_SET(fd_udp, &rfds);
				FD_SET(0, &rfds);
				FD_SET(afd, &rfds);
				max_list = 0;
				for (i = 0; i < size_list; i++) {
					if (socket_list[i] != 0) {
						if (socket_list[i] > max_list) {
							max_list = socket_list[i];
						}
						FD_SET(socket_list[i], &rfds);
					}
				}
				maxfd = max(max(max(max(fd_tcp, fd_udp), 0), max_list), afd);
				break;
		}
		counter = select(maxfd + 1, &rfds, (fd_set*)NULL, (fd_set*)NULL, (struct timeval *)NULL);

		/* Counter: Returns the number of file descriptors ready. */
		if (counter <= 0) {
			Closes_Frees(fd_tcp, fd_udp, afd, &node, res_tcp, res_udp, message, IPepred, PORTepred, fd_suc, fd_pred, socket_list, size_list);
			fprintf(stderr, "error: select: %s\n", strerror(errno));
			return 0;
		}

		for (;counter;--counter) {
			switch (state) {
				case idle:
					flag = 0;

					/* Let's process keyboard command */
					if (FD_ISSET(0, &rfds)) {
						FD_CLR(0, &rfds);
						afd = 0;
						flag = 0;
						state = busy;
						break;
					}

					/* Let's process a TCP new connection */
					else if (FD_ISSET(fd_tcp, &rfds)) {
						FD_CLR(fd_tcp, &rfds);
						addrlen_tcp = sizeof(addr_tcp);
						if ((newfd = accept(fd_tcp, &addr_tcp, &addrlen_tcp)) == -1) {
							fprintf(stderr, "error: accept: %s\n", strerror(errno));
							Closes_Frees(fd_tcp, fd_udp, afd, &node, res_tcp, res_udp, message, IPepred, PORTepred, fd_suc, fd_pred, socket_list, size_list);
							return 0;
						}
						afd = newfd;
						/* Add socket associated with new connection to the socket tcp receivement list */
						socket_list = addlist(socket_list, size_list, afd);
						flag = 1;
						state = busy;
						break;
					}

					/* Let's process UDP command */
					else if (FD_ISSET(fd_udp, &rfds)) {
						FD_CLR(fd_udp, &rfds);
						addrlen_udp = sizeof(addr_udp);
						newfd = fd_udp;
						afd = newfd;
						flag = 2;
						state = busy;
						break;
					}
					state = busy;
					/* Caso o descritor ativo seja um dos que já está na lista, põe a flag a 60 */
					flag = 60;
					break;
				case busy:
					if (FD_ISSET(afd, &rfds) || flag == 60 || flag == 1) { 
						
						/* Let's process keyboard command */
						if (afd == 0 && flag != 60 && flag != 1) { 
							FD_CLR(afd, &rfds);
							if (fgets (buffer, 129, stdin) != NULL ) {
								/* printf("Received the message: %s", buffer); */
								readcode = ReadComand(ptrnode, buffer, flag, addr_udp, &sequence_num, &addr_udp_epred,&print, &afd, &key_epred, 
														&IPepred, &PORTepred, &fd_pred, &fd_suc, socket_list, size_list, ack_hability, &saveguard_efnd);
								ack_hability = CheckACKhability(ptrnode);
								/* Exit command received*/
								if (readcode == 60 || readcode == 50) {
									Closes_Frees(fd_tcp, fd_udp, afd, &node, res_tcp, res_udp, message, IPepred, PORTepred, fd_suc, fd_pred, socket_list, size_list);
									return 0;
								}
								else if (readcode == -2) {
									fprintf(stderr, "error: Command not valid\n");
								}
							}
							else {
								fprintf(stderr, "error: read: %s\n", strerror(errno));
							}
							state = idle;
						}
						
						/* Let's process TCP command */
						else if (flag == 1 || flag == 60) {
							/* Process the active socket in the list */
							for (i = 0; i < size_list; i++) {
								if (socket_list[i] != 0 && FD_ISSET(socket_list[i], &rfds)) {
									flag_select = 1;
									readcode = 0;
									nbytes = 128;
									nleft = nbytes;
									ptrr = buffer;
									timeout = 100000;
									while (nleft > 0 && timeout > 0) {
										timeout--;
										nread = read(socket_list[i], ptrr, nleft);
										if (nread == -1) {
											fprintf(stderr, "error: read: %s\n", strerror(errno));
											errcode = -1;
										}
										else if (nread == 0) {
											state = idle;
											nleft = 0;
											flag_select = 0;
											/* If the socket is active without anything to read: remove him from the list */
											socket_list = removelist(socket_list, size_list, socket_list[i]);
											break;
										}
										else {
											nleft -= nread;
											ptrr += nread;
											if (strchr(buffer, '\n') != NULL || buffer[nread - 1] == '\n' || (buffer[nread - 2] == '\n' && buffer[nread - 1] == '\0')) {
												state = idle;
												break;
											}
										}
									}
									if (timeout < 0) {
										flag_select = 0;
									}
									if (flag_select == 1) {
										state = idle;
										nread = nbytes - nleft;
										if (errcode != -1) {
											buffer[nread] = '\0';
											/* printf("Received the message: %s", buffer); */
											readcode = ReadComand(ptrnode, buffer, flag, addr_udp, &sequence_num, &addr_udp_epred, &print, 
																 &afd, &key_epred, &IPepred, &PORTepred, &fd_pred, &fd_suc, socket_list, 
																 size_list, ack_hability, &saveguard_efnd);
											ack_hability = CheckACKhability(ptrnode);
											if (readcode == -2) {
												fprintf(stderr, "error: Command not valid\n");
											}
											else if (readcode == 60 || readcode == 50) {
												Closes_Frees(fd_tcp, fd_udp, afd, &node, res_tcp, res_udp, message, IPepred, PORTepred, fd_suc, fd_pred, socket_list, size_list);
												return 0;
											}
											state = idle;
										}
										/* Send EPRED command */
										if (readcode == 99) {
											message = free_string(message);
											message = (char *) calloc((128 + 1) , sizeof(char));
											if (message == NULL) {
												Closes_Frees(fd_tcp, fd_udp, afd, &node, res_tcp, res_udp, message, IPepred, PORTepred, fd_suc, fd_pred, socket_list, size_list);
												return 0;
											}
											id_char = intToAscii_space(key_epred);
											if (id_char == NULL) {
												message = free_string(message);
												Closes_Frees(fd_tcp, fd_udp, afd, &node, res_tcp, res_udp, message, IPepred, PORTepred, fd_suc, fd_pred, socket_list, size_list);
												return 0;
											}
											message = strcpy(message,"EPRED");
											message = strcat(message, id_char);
											message = strcat(message, " ");
											message = strcat(message, IPepred);
											message = strcat(message, " ");
											message = strcat(message, PORTepred);
											id_char = free_string(id_char);
											addrlen_udp = sizeof(addr_udp);
											errcode = sendto(fd_epred2, message, strlen(message), 0, &addr_epred2, addrlen_epred2);
											if (errcode == -1) {
												fprintf(stderr, "Error\n");
											}
											saveguard_efnd = 0;
										}
									}
									flag_select = 1;
									FD_CLR(socket_list[i], &rfds);
								}
							}
							state = idle;
						}

						/* Let's process UDP command */
						else if (flag == 2) {
							FD_CLR(afd, &rfds);
							readcode = 0;
							addrlen_udp = sizeof(addr_udp);
							nread = recvfrom(afd, buffer, 128, 0, &addr_udp, &addrlen_udp);
							if (nread == -1) {
								fprintf(stderr, "error: recvfrom: %s\n", strerror(errno));
							}
							else {
								/* Send ACK to who sent the message*/
								buffer[nread] = '\0';
								if (ack_hability) {
									 /* printf("Received the message: %s \n", buffer); */
								}
								if (strcmp("ACK" , buffer) != 0 && ack_hability == 1) {
									errcode = sendto(afd, "ACK", strlen("ACK"), 0, &addr_udp, addrlen_udp);
									if (errcode == -1) {
										fprintf(stderr, "Error: can't send ACK\n");
									}
									else {
										readcode = ReadComand(ptrnode, buffer, flag, addr_udp, &sequence_num, &addr_udp_epred, 
															 &print, &afd, &key_epred, &IPepred, &PORTepred, &fd_pred, &fd_suc, 
															 socket_list, size_list, ack_hability, &saveguard_efnd);
										if (readcode == 60 || readcode == 50) {
											Closes_Frees(fd_tcp, fd_udp, afd, &node, res_tcp, res_udp, message, IPepred, PORTepred, fd_suc, fd_pred, socket_list, size_list);
											return 0;
										}
										if (saveguard_efnd == 1) {
											fd_epred2 = afd;
											addr_epred2 = addr_udp;
											addrlen_epred2 = addrlen_udp;
										}
										saveguard_efnd = 0;
										ack_hability = CheckACKhability(ptrnode);
										if (readcode == -2 && ack_hability == 1) {
											fprintf(stderr, "error: Command not valid\n");
										}
									}
								}
								else {
									readcode = ReadComand(ptrnode, buffer, flag, addr_udp, &sequence_num, &addr_udp_epred, &print, 
														 &afd, &key_epred, &IPepred, &PORTepred, &fd_pred, &fd_suc, socket_list, 
														 size_list, ack_hability, &saveguard_efnd);
									/* if (saveguard_efnd == 1) {
										fd_epred2 = afd;
										addr_epred2 = addr_udp;
										addrlen_epred2 = addrlen_udp;
									} */
									if (readcode == 60 || readcode == 50) {
										Closes_Frees(fd_tcp, fd_udp, afd, &node, res_tcp, res_udp, message, IPepred, PORTepred, fd_suc, fd_pred, socket_list, size_list);
										return 0;
									}
									saveguard_efnd = 0; 
									ack_hability = CheckACKhability(ptrnode);
									if (readcode == -2 && ack_hability == 1) {
										fprintf(stderr, "error: Command not valid\n");
									}
								}
								/* Send EPRED command */
								if (readcode == 99) {
									message = free_string(message);
									message = (char *) calloc((128 + 1) , sizeof(char));
									if (message == NULL) {
										Closes_Frees(fd_tcp, fd_udp, afd, &node, res_tcp, res_udp, message, IPepred, PORTepred, fd_suc, fd_pred, socket_list, size_list);
										return 0;
									}
									id_char = intToAscii_space(key_epred);
									if (id_char == NULL) {
										message = free_string(message);
										Closes_Frees(fd_tcp, fd_udp, afd, &node, res_tcp, res_udp, message, IPepred, PORTepred, fd_suc, fd_pred, socket_list, size_list);
										return 0;
									}
									message = strcpy(message,"EPRED");
									message = strcat(message, id_char);
									message = strcat(message, " ");
									message = strcat(message, IPepred);
									message = strcat(message, " ");
									message = strcat(message, PORTepred);
									id_char = free_string(id_char);
									addrlen_udp = sizeof(addr_udp);
									errcode = sendto(fd_epred2, message, strlen(message), 0, &addr_epred2, addrlen_epred2);
									if (errcode == -1) {
										fprintf(stderr, "Error\n");
									}
									saveguard_efnd = 0;
								}
							}
							state = idle;
						}
					}
					else {
						printf("Error: Socket died\n");	
						Closes_Frees(fd_tcp, fd_udp, afd, &node, res_tcp, res_udp, message, IPepred, PORTepred, fd_suc, fd_pred, socket_list, size_list);
					}
					flag = 0;
			}
		}
	}
	
	Closes_Frees(fd_tcp, fd_udp, afd, &node, res_tcp, res_udp, message, IPepred, PORTepred, fd_suc, fd_pred, socket_list, size_list);
    return 0;
}
