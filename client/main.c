/*
** talker.c -- a datagram "client" demo
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

#define MAXBUFLEN 100
#define SERVERPORT "4950"	// the port users will be connecting to

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

void add_js_int_array_field_packet(json_object *jobj, char key[],int pos[], const char **out) {
    jobj = json_tokener_parse(*out);
    json_object *field_array = json_object_new_array();
    for (int i = 0; i < 2; i++) {
        json_object_array_add(field_array, json_object_new_int(pos[i]));
    }
    json_object_object_add(jobj, key, field_array);
    *out = json_object_to_json_string(jobj);
}

void send_packet(int sockfd, struct addrinfo *p, char *packet, int *numbytes) {
    if ((*numbytes = sendto(sockfd, packet, strlen(packet), 0,
                            p->ai_addr, p->ai_addrlen)) == -1) {
        perror("client: sendto");
        exit(1);
    }
    printf("client: sent %s\n", packet);
}

void extract_js_packet(json_object *jobj, const char *data, char key[],char **out) {
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


void recv_packet(int sockfd, struct sockaddr *server_addr, char *buffer, int *numbytes) {
    printf("client: waiting to recvfrom...\n");
    socklen_t addr_len = sizeof server_addr;
    if ((*numbytes = recvfrom(sockfd, buffer, MAXBUFLEN - 1, 0,
                            (struct sockaddr *) &server_addr, &addr_len)) == -1) {
        perror("talke: recvfrom");
        exit(1);
    }
}

void print_json_matrix(json_object *jobj) {
    if (!jobj || !json_object_is_type(jobj, json_type_array)) {
        printf("Invalid JSON matrix\n");
        return;
    }

    for (int i = 0; i < json_object_array_length(jobj); i++) {
        json_object *row = json_object_array_get_idx(jobj, i);
        if (!row || !json_object_is_type(row, json_type_array)) {
            continue;
        }

        for (int j = 0; j < json_object_array_length(row); j++) {
            json_object *elem = json_object_array_get_idx(row, j);
            printf(" %d", json_object_get_int(elem));
            if (j < json_object_array_length(row) - 1) {
            }
        }
        printf(" %s\n", (i < json_object_array_length(jobj) - 1) ? "" : "");
    }
}

int main(int argc, char *argv[]) {
    char *verbs[] = {"\"START\"", "\"READY\"", "\"TURN\"", "\"MOVE\"", "\"TERMINATE\"", "\"ASSIGNED\""};
    char *verbsServer[] = {"START", "READY", "TURN", "MOVE", "TERMINATE", "ASSIGNED"};
    json_object *jobj = json_object_new_object();

    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int numbytes;
    struct sockaddr_storage server_addr;
    int asignedPlayer;
    int checkPlayer;
    const char *exData;
    const char *packet;
    char buf[MAXBUFLEN];
    int pos[2];


    if (argc != 2) {
        fprintf(stderr, "usage: talker hostname message\n");
        exit(1);
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET6; // set to AF_INET to use IPv4
    hints.ai_socktype = SOCK_DGRAM;

    if ((rv = getaddrinfo(argv[1], SERVERPORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    p = servinfo;
    if (p == NULL) {
        fprintf(stderr, "talker: failed to create socket\n");
        return 2;
    }
    if ((sockfd = socket(p->ai_family, p->ai_socktype,
                         p->ai_protocol)) == -1) {
        perror("talker: socket");
        exit(-1);
    }


    create_js_paket(jobj, verbsServer[1], &packet);
    send_packet(sockfd, p, packet, &numbytes);

    recv_packet(sockfd, &server_addr, buf, &numbytes);
    buf[numbytes] = '\0';

    printf("client: recieved: %s\n", buf);

    extract_js_packet(jobj, buf, "verb", &exData);

    if (strcmp(exData, verbs[5]) == 0) {
        extract_js_packet_int(jobj, buf, "msg", &asignedPlayer);
    }

    printf("client: playerID %d\n", asignedPlayer);

    while (true) {
        recv_packet(sockfd, &server_addr, buf, &numbytes);
        buf[numbytes] = '\0';

        extract_js_packet(jobj, buf, "verb", &exData);
        if (strcmp(exData, verbs[0]) == 0) {
            extract_js_packet_int(jobj, buf, "msg", &checkPlayer);
            if (checkPlayer == asignedPlayer) {
                //printing the map
                extract_js_packet(jobj, buf, "MAP", &packet);
                json_object *jmap =  json_tokener_parse(packet);
                print_json_matrix(jmap);
                //entering the position
                create_js_paket_msg_int(jobj, verbsServer[3], asignedPlayer, &packet);
                printf("Enter first position: \n");
                scanf("%d", &pos[0]);
                printf("Enter second position: \n");
                scanf("%d", &pos[1]);
                add_js_int_array_field_packet(jobj, "pos", pos, &packet);
                send_packet(sockfd, p, packet, &numbytes);
            }
        }
        else if (strcmp(exData, verbs[2]) == 0) {
            extract_js_packet_int(jobj, buf, "msg", &checkPlayer);
            if (checkPlayer == asignedPlayer) {
                //printing the map
                extract_js_packet(jobj, buf, "MAP", &packet);
                json_object *jmap =  json_tokener_parse(packet);
                print_json_matrix(jmap);
                //entering the position
                create_js_paket_msg_int(jobj, verbsServer[3], asignedPlayer, &packet);
                printf("Enter first position: \n");
                scanf("%d", &pos[0]);
                printf("Enter second position: \n");
                scanf("%d", &pos[1]);
                add_js_int_array_field_packet(jobj, "pos", pos, &packet);
                send_packet(sockfd, p, packet, &numbytes);
            }
        }
        else if (strcmp(exData, verbs[4]) == 0) {
            extract_js_packet_int(jobj, buf, "msg", &checkPlayer);
            if (checkPlayer == asignedPlayer) {
                //printing the map
                extract_js_packet(jobj, buf, "MAP", &packet);
                json_object *jmap =  json_tokener_parse(packet);
                print_json_matrix(jmap);
                //entering the position
                break;
            }
        }

    }



    json_object_put(jobj);
    freeaddrinfo(servinfo);
    close(sockfd);

    return 0;
}
