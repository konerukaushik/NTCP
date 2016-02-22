#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/time.h>    
#include "router.h"

int main() {
    int                     router_fd;
    struct sockaddr_in      router_addr;
    char                    buffer[256];
    int                     i=0, readn =0, written = 0;
    struct hostent*         hosta;
    struct in_addr*         ip_Address;
    int                     y = 0;
    fd_set                  read_fd;
    struct timeval          timeout;
    int                     activity = 0, missing_seq =0;
    char                    temp[256];

    LOG("Initialized Host A")
    router_fd = socket(AF_INET, SOCK_STREAM, IP_PROTOCOL); 
    if ( router_fd < 0 ) {
        printf(" Unable to Create a socket for Router\n");
        return ERROR;
    }

    LOG("Created Socket ..")
    bzero((char *) &router_addr, sizeof(router_addr));
    
    hosta = gethostbyname("localhost");
    ip_Address = (struct in_addr*)hosta->h_addr_list[0];
    
    router_addr.sin_family = AF_INET;
    router_addr.sin_port   = htons(ROUTER_PORT);
    router_addr.sin_addr.s_addr = ip_Address->s_addr; 

    if (connect(router_fd, (struct sockaddr *)&router_addr,sizeof(router_addr)) < 0) {
        printf("Unable to Connect to router");
        return ERROR;
    }

    FD_ZERO(&read_fd);
    FD_SET(router_fd, &read_fd);

    LOG("Connected to Router");

    /* 
     * As HOST A is connected to Router 
     * Following code is to pumpin the traffic and receive acknowledgement
     */

    while(y++ < Y) {
        LOG("Filling in Buffer ...")
        bzero(buffer, 256);
        sprintf(buffer, "Message Number %d",i++);
        printf("PKT --> %s \n",buffer);
        /* Start a Timer */

resend:
	sleep(1);
        /* Write to router socket */
        written = write(router_fd, buffer, strlen(buffer));
        if (written < 0) {
            LOG("Unable to Write to Router ");
            continue;
        }

        timeout.tv_sec  = 0;//`TIMEOUT_SEC;
        timeout.tv_usec = 0;

        /* Wait for Acknowledgement */
        FD_ZERO(&read_fd);
        FD_SET(router_fd, &read_fd);

        activity = select(router_fd+1, &read_fd, NULL, NULL, &timeout);

        if (activity > 0) {
            if (FD_ISSET(router_fd, &read_fd)) {
                bzero(buffer, 256);
                readn = read(router_fd, buffer, 256);
                if ( readn < 0 ) {
                    LOG("Unable to Read from Router");
                    continue;
                }
                sscanf(buffer, "%*s %*s %*s %*s %s %d", temp, &missing_seq);
                printf("Message from Hostb --> %s\n",buffer);
                printf("Resending Message %d\n",missing_seq);
                bzero(buffer,256);
                sprintf(buffer, "Message Number %d",missing_seq);
                printf("Sending Missed Packet --> %s\n",buffer);
                goto resend;
            }
        }
    }

    /* Once Done Check any last packet is missing If Missing resend it */
    while(1) {
    sleep(1);
    bzero(buffer, 256);
    readn = read(router_fd, buffer, 256);
    if ( readn < 0) {
        LOG("Unable to read from router");
        return 0;
    }
        sscanf(buffer, "%*s %*s %*s %*s %s %d", temp, &missing_seq);
        printf("Message from Hostb --> %s \n", buffer);
        printf("Resending Message %d\n",missing_seq);
        bzero(buffer,256);
        sprintf(buffer, "Message Number %d",missing_seq);
        printf("Sending Missed Packet --> %s\n",buffer);
        if (write(router_fd, buffer, strlen(buffer)) < 0) {
            LOG("unable to write");
        }
    }

    return 0;
}
