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

void create_js_paket(json_object *jobj, char verb[],  const char **out) {

    json_object_object_add(jobj, "verb", json_object_new_string(verb));

     *out = json_object_to_json_string(jobj);
}

void create_js_paket_msg(json_object *jobj, char verb[], char msg[], const char **out) {
    json_object_object_add(jobj, "verb", json_object_new_string(verb));
    json_object_object_add(jobj, "msg", json_object_new_string(msg));

    *out = json_object_to_json_string(jobj);
}

void send_packet(int sockfd,struct sockaddr *client_addr, char * packet, int *numbytes) {
    printf("listener: sending %s\n",packet);
    socklen_t addr_len = sizeof client_addr;
    if ((*numbytes = sendto(sockfd, packet, strlen(packet), 0,
              (struct sockaddr *) &client_addr, addr_len)) == -1) {
        perror("listiner: sendto");
        exit(1);
              }
}

void recv_packet(int sockfd,struct sockaddr * client_addr, char * buffer, int *numbytes) {
    printf("listener: waiting to recvfrom...\n");
    socklen_t addr_len = sizeof client_addr;
    if ((*numbytes = recvfrom(sockfd, buffer, MAXBUFLEN - 1, 0,
                             (struct sockaddr *) &client_addr, &addr_len)) == -1) {
        perror("recvfrom");
        exit(1);
                             }
}

int main(void) {

    char *verbs[] = {"START","READY", "TURN", "MOVE", "END"};
    json_object *jobj = json_object_new_object();


    int sockfd; //socket file descriptor aka packet  retrieved
    struct addrinfo hints, *servinfo, *p; //hints saves
    int rv;
    int numbytes;
    struct sockaddr_storage their_addr;
    char buf[MAXBUFLEN];
    char s[INET6_ADDRSTRLEN];

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
    socklen_t addr_len;
    addr_len = sizeof their_addr;
    if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN - 1, 0,
                             (struct sockaddr *) &their_addr, &addr_len)) == -1) {
        perror("recvfrom");
        exit(1);
                             }

    printf("listener: got packet from %s\n",
           inet_ntop(their_addr.ss_family,
                     get_in_addr((struct sockaddr *) &their_addr),
                     s, sizeof s));
    printf("listener: packet is %d bytes long\n", numbytes);
    buf[numbytes] = '\0';
    printf("listener: packet contains \"%s\"\n", buf);

    char argc[] = "ACK";
    printf("listener: sending %s\n", argc);
    if ((numbytes = sendto(sockfd, argc, strlen(argc), 0,
              (struct sockaddr *) &their_addr, addr_len)) == -1) {
        perror("listiner: sendto");
        exit(1);
              }




    // free the memory after the interaction is done
    freeaddrinfo(servinfo);
    json_object_put(jobj);
    close(sockfd);

    return 0;
}
