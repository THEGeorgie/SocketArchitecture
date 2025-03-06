/*
** listener.c -- a datagram sockets "server" demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <json-c/json.h>
#include <stdbool.h>

#define MYPORT "4950"	// the port users will be connecting to

#define MAXBUFLEN 100	//buffer max size of 100B

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
//sockaddr describes a socket address (Address family(Ipv4,Ipv6 etc.) and address)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in *) sa)->sin_addr);
    }

    return &(((struct sockaddr_in6 *) sa)->sin6_addr);
}

void create_js_paket(json_object *jobj, char verb[], const char **out) {
    json_object_object_add(jobj, "verb", json_object_new_string(verb));

    *out = json_object_to_json_string(jobj);
}

void create_js_paket_msg(json_object *jobj, char verb[], char msg[], const char **out) {
    json_object_object_add(jobj, "verb", json_object_new_string(verb));
    json_object_object_add(jobj, "msg", json_object_new_string(msg));

    *out = json_object_to_json_string(jobj);
}

void create_js_paket_msg_int(json_object *jobj, char verb[], int msg, const char **out) {
    json_object_object_add(jobj, "verb", json_object_new_string(verb));
    json_object_object_add(jobj, "msg", json_object_new_int(msg));

    *out = json_object_to_json_string(jobj);
}

void add_js_object_packet_map(json_object *jobj, char verb[], int rows, int cols, int map[3][3], const char **out) {
    jobj = json_tokener_parse(*out);
    json_object *outer_array = json_object_new_array(); // Create main array

    for (int i = 0; i < rows; i++) {
        json_object *row_array = json_object_new_array(); // Create a row (inner array)
        for (int j = 0; j < cols; j++) {
            json_object_array_add(row_array, json_object_new_int(map[i][j])); // Add values
        }
        json_object_array_add(outer_array, row_array); // Add row to main array
    }

    json_object_object_add(jobj, verb, outer_array); // Attach the array to the JSON object
    *out = json_object_to_json_string(jobj);
}

void extract_js_packet(json_object *jobj, const char *data, char key[], char **out) {
    jobj = json_tokener_parse(data);
    if (!jobj) {
        printf("Failed to parse JSON\n");
        *out = NULL;
        return;
    }
    json_object *jobjC;
    if (json_object_object_get_ex(jobj, key, &jobjC)) {
        *out = json_object_to_json_string(jobjC);
    } else {
        printf("Key not found: %s\n", key);
        *out = NULL;
    }
}

void extract_js_packet_int(json_object *jobj, const char *data, char key[], int *out) {
    jobj = json_tokener_parse(data);
    if (!jobj) {
        printf("Failed to parse JSON\n");
        out = -1;
        return;
    }
    json_object *jobjC;
    if (json_object_object_get_ex(jobj, key, &jobjC)) {
        *out = json_object_get_int(jobjC);
    } else {
        printf("Key not found: %s\n", key);
        out = -1;
    }
}

void send_packet(int sockfd, struct sockaddr *client_addr, char *packet, int *numbytes) {
    printf("listener: sending %s\n", packet);
    socklen_t addr_len = sizeof client_addr;

    if ((*numbytes = sendto(sockfd, packet, strlen(packet), 0,
                            client_addr, addr_len)) == -1) {
        perror("listiner: sendto");
        exit(1);
    }
}

void extract_js_array(json_object *jobj, int *out) {
    if (!jobj || !json_object_is_type(jobj, json_type_array)) {
        printf("Invalid JSON array\n");
        return;
    }

    if (json_object_array_length(jobj) != 2) {
        printf("Expected array of size 2, but got %zu\n", json_object_array_length(jobj));
        return;
    }

    for (size_t i = 0; i < 2; i++) {
        json_object *jval = json_object_array_get_idx(jobj, i);
        if (!jval || !json_object_is_type(jval, json_type_int)) {
            printf("Invalid element at index %zu\n", i);
            out[i] = 0; // Default to 0 in case of error
        } else {
            out[i] = json_object_get_int(jval);
        }
    }
}

void recv_packet(int sockfd, struct sockaddr *client_addr, char *buffer, int *numbytes) {
    printf("listener: waiting to recvfrom...\n");
    socklen_t addr_len = sizeof client_addr;

    printf("listener: addrlen %d\n", addr_len);
    if ((*numbytes = recvfrom(sockfd, buffer, MAXBUFLEN - 1, 0,
                              client_addr, &addr_len)) == -1) {
        perror("recvfrom");
        exit(1);
    }
}

int check_game_state(int map[3][3]) {
    // Check rows and columns
    for (int i = 0; i < 3; i++) {
        if (map[i][0] != 0 && map[i][0] == map[i][1] && map[i][1] == map[i][2])
            return map[i][0]; // Row win
        if (map[0][i] != 0 && map[0][i] == map[1][i] && map[1][i] == map[2][i])
            return map[0][i]; // Column win
    }

    // Check diagonals
    if (map[0][0] != 0 && map[0][0] == map[1][1] && map[1][1] == map[2][2])
        return map[0][0]; // Main diagonal win
    if (map[0][2] != 0 && map[0][2] == map[1][1] && map[1][1] == map[2][0])
        return map[0][2]; // Anti-diagonal win

    // Check for a draw (if board is full and no winner)
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            if (map[i][j] == 0)
                return 0; // No winner yet (still empty cells)
        }
    }

    return 3; // It's a draw
}

int main(void) {
    char *verbs[] = {"\"START\"", "\"READY\"", "\"TURN\"", "\"MOVE\"", "\"TERMINATE\"", "\"ASSIGNED\"", "\"MAP\""};
    char *verbsClient[] = {"START", "READY", "TURN", "MOVE", "TERMINATE", "ASSIGNED", "MAP"};
    json_object *jobj = json_object_new_object();


    int sockfd; //socket file descriptor aka packet  retrieved
    struct addrinfo hints, *servinfo, *p; //hints saves
    int rv;
    int numbytes;
    struct sockaddr_storage their_addr;
    struct sockaddr_storage clients[2];
    char buf[MAXBUFLEN];
    char s[INET6_ADDRSTRLEN];
    const char *packet;
    socklen_t addr_len;
    addr_len = sizeof their_addr;
    const char *exData;
    int map[3][3] = {
        {0, 0, 0},
        {0, 0, 0},
        {0, 0, 0}
    };

    int players[] = {1, 2};
    int nextPlayer;
    int playerPos[2];
    bool *playerUsed[] = {false, false};


    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET6; // set to AF_INET to use IPv4
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo("localhost", MYPORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    p = servinfo;
    //sanity check to se if the gotten addrinfo struct is null
    if (p == NULL) {
        fprintf(stderr, "listener: failed to bind socket\n");
        return 2;
    }
    if ((sockfd = socket(p->ai_family, p->ai_socktype,
                         p->ai_protocol)) == -1) {
        perror("listener: socket");
        exit(-1);
    }
    if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
        //binds the socket to the address
        close(sockfd);
        perror("listener: bind");
        exit(-2);
    }

    printf("listener: waiting to recvfrom...\n");

    //Players joining and assigning hem ids and saving their addresses.
    for (int i = 0; i < 2; i++) {
        if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN - 1, 0,
                                 (struct sockaddr *) &their_addr, &addr_len)) == -1) {
            perror("recvfrom");
            exit(1);
        }

        buf[numbytes] = '\0';
        printf("listener: got packet from %s\n",
               inet_ntop(their_addr.ss_family,
                         get_in_addr((struct sockaddr *) &their_addr),
                         s, sizeof s));
        printf("listener: packet contains \"%s\"\n", buf);

        extract_js_packet(jobj, buf, "verb", &exData);
        if (strcmp(verbs[1], exData) == 0) {
            for (int i = 0; i < sizeof(players); i++) {
                if (playerUsed[i] == false) {
                    playerUsed[i] = true;
                    clients[i] = their_addr;
                    create_js_paket_msg_int(jobj, verbsClient[5], players[i], &packet);
                    break;
                }
            }
        } else if (strcmp(verbs[4], exData) == 0) {
            printf("Exiting...\n");
            break;
        }

        printf("listener: sending %s\n", packet);
        if ((numbytes = sendto(sockfd, packet, strlen(packet), 0,
                               (struct sockaddr *) &their_addr, addr_len)) == -1) {
            perror("listiner: sendto");
            exit(1);
        }
    }
    //starting the game
    for (int i = 0; i < 2; i++) {
        create_js_paket_msg_int(jobj, verbsClient[0], players[0], &packet);
        add_js_object_packet_map(jobj, verbsClient[6], 3, 3, map, &packet);

        printf("listener: sending %s\n", packet);
        if ((numbytes = sendto(sockfd, packet, strlen(packet), 0,
                               (struct sockaddr *) &clients[i], addr_len)) == -1) {
            perror("listiner: sendto");
            exit(1);
        }
    }
    bool running = true;

    while (running) {
        for (int i = 0; i < 2; i++) {
            if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN - 1, 0,
                                     (struct sockaddr *) &clients[i], &addr_len)) == -1) {
                perror("recvfrom");
                exit(1);
            }
            buf[numbytes] = '\0';
            printf("listener: got packet from %s\n", buf);

            extract_js_packet(jobj, buf, "pos", &exData);
            json_object *jPos = json_tokener_parse(exData);
            extract_js_array(jPos, &playerPos);
            printf("listener: player pos got: %d , %d\n", playerPos[0], playerPos[1]);
            map[playerPos[0]][playerPos[1]] = players[i];

            int result = check_game_state(map);
            switch (result) {
                case 1:
                    create_js_paket_msg_int(jobj, verbsClient[4], 1, &packet);
                    add_js_object_packet_map(jobj, verbsClient[6], 3, 3, map, &packet);
                    for (int j = 0; j < 2; j++) {
                        printf("listener: sending %s\n", packet);
                        if ((numbytes = sendto(sockfd, packet, strlen(packet), 0,
                                               (struct sockaddr *) &clients[j], addr_len)) == -1) {
                            perror("listiner: sendto");
                            exit(1);
                        }
                    }
                    running != running;
                break;
                case 2:
                    create_js_paket_msg_int(jobj, verbsClient[4], 2, &packet);
                    add_js_object_packet_map(jobj, verbsClient[6], 3, 3, map, &packet);
                    for (int j = 0; j < 2; j++) {
                        printf("listener: sending %s\n", packet);
                        if ((numbytes = sendto(sockfd, packet, strlen(packet), 0,
                                               (struct sockaddr *) &clients[j], addr_len)) == -1) {
                            perror("listiner: sendto");
                            exit(1);
                        }
                    }
                    running != running;
                break;
                case 3:
                    create_js_paket_msg(jobj, verbsClient[4], "DRAW", &packet);
                    add_js_object_packet_map(jobj, verbsClient[6], 3, 3, map, &packet);
                    for (int j = 0; j < 2; j++) {
                        printf("listener: sending %s\n", packet);
                        if ((numbytes = sendto(sockfd, packet, strlen(packet), 0,
                                               (struct sockaddr *) &clients[j], addr_len)) == -1) {
                            perror("listiner: sendto");
                            exit(1);
                        }
                    }
                    running != running;
                break;
            }



            if (i == 0) {
                nextPlayer = 1;
            }else if (i == 1) {
                nextPlayer = 0;
            }

            create_js_paket_msg_int(jobj, verbsClient[2], players[nextPlayer], &packet);
            add_js_object_packet_map(jobj, verbsClient[6], 3, 3, map, &packet);
            printf("listener: sending %s\n", packet);
            if ((numbytes = sendto(sockfd, packet, strlen(packet), 0,
                                   (struct sockaddr *) &clients[nextPlayer], addr_len)) == -1) {
                perror("listiner: sendto");
                exit(1);
            }
        }
    }

    // free the memory after the interaction is done
    freeaddrinfo(servinfo);
    json_object_put(jobj);
    close(sockfd);

    return 0;
}
