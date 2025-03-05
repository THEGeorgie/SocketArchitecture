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

void send_packet(int sockfd, struct addrinfo *p, char *packet, int *numbytes) {
    if ((*numbytes = sendto(sockfd, packet, strlen(packet), 0,
                            p->ai_addr, p->ai_addrlen)) == -1) {
        perror("client: sendto");
        exit(1);
    }
    printf("client: sent %s\n", packet);
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


int main(int argc, char *argv[]) {
    char *verbs[] = {"START", "READY", "TURN", "MOVE", "END"};
    json_object *jobj = json_object_new_object();

    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int numbytes;
    struct sockaddr_storage server_addr;

    if (argc != 3) {
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


    const char *msg;
    create_js_paket(jobj, verbs[1], &msg);
    send_packet(sockfd, p, msg, &numbytes);

    char buf[MAXBUFLEN];
    recv_packet(sockfd, &server_addr, buf, &numbytes);
    buf[numbytes] = '\0';
    printf("client: received %d bytes message %s\n", numbytes, buf);


    json_object_put(jobj);
    freeaddrinfo(servinfo);
    close(sockfd);

    return 0;
}
