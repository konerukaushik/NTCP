#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include "router.h"

/* To Maintain Router FD */
int router_fd;
long missing_seq[256][2];

#define WAIT_TIME 10000

static void * host_monitor_sequence(void *thread_args) {

    unsigned long    present_time = (unsigned)time(NULL);
    int              i = 0;
    char             buffer[256];

    while(1) {
        sleep(1);
        for(i = 0; i < 256; i++) {
            bzero(buffer,256);
            sleep(1);
            if (missing_seq[i][0] == 0) {
                break;
            }
            if (present_time - missing_seq[i][1]  >= WAIT_TIME) {
                sprintf(buffer, "Missing Message with Seq Number %lu\n",missing_seq[i][0]);
                printf("Requesting for Packet with Seq Number %lu\n",missing_seq[i][0]);
                if (write(router_fd, buffer, strlen(buffer)) < 0) {
                    LOG("Unable to write to router back");
                }
            }
        }
    }
}

int host_missing_packet(int seq_num) {
    int          i = 0;

    for(i=0; i < 256; i++) {
        if (missing_seq[i][0] == seq_num) {
            /* Remove missing seq num from array */
            for (; i <256 ;i++) {
                missing_seq[i][0] = missing_seq[i+1][0];
                missing_seq[i][1] = missing_seq[i+1][1];
                if (missing_seq[i][0] == 0) {
                    break;
                }
            }
            return 1;
        } else if (missing_seq[i][0] == 0) {
            return 0;
        } 
    }
    return 0;
}


int host_get_message_number(const char *message) {
    int    number  = -1;
    char    temp[20];

    sscanf(message, "%s %*s %d",temp,&number);

    return number;
}

void host_missing_messages(int prev, int now) {
    int         i,j; 
    
    for(i = prev+1; i< ( now); i++) {
        for (j=0;j<256;j++) {
            if (missing_seq[j][0] == 0 ) {
                missing_seq[j][0] = i;
                missing_seq[j][1] = (unsigned)time(NULL);
                break;
            }
        }

    }

    return;
}

int main() {
    struct sockaddr_in      router_addr;
    char                    buffer[256];
    int                     i=0, readn =0, written = 0;
    struct hostent*         hosta;
    struct in_addr*         ip_Address;
    int                     initial_step = 0;
    int                     number_prev = 0, number_present = 0;
    
    /* Pthread informtion */
    pthread_attr_t          attr;
    int                     stack_size = 0x100000;
    pthread_t               thread_id;

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

    LOG("Connected to Router");

    /* Create a pthread to check any missing seq timeout */
    if (pthread_attr_init(&attr)) {
        LOG("Unable to create Pthread attribute");
        return ERROR;
    }
    
    if (pthread_attr_setstacksize(&attr, stack_size)) {
        LOG("Unable to set stack size");
        return ERROR;
    }

    if (pthread_create(&thread_id, &attr, &host_monitor_sequence, NULL )) {
        LOG("Unable to create monitor thread");
        return ERROR;
    }

    /* 
     * As HOST B is connected to Router 
     * Following code is to pumpin the traffic and receive acknowledgement
     */

    initial_step = 0;

    bzero(missing_seq,10);
    while(1) {
        /* Start a Timer */

        LOG("Waiting to Listen from Router ")
        bzero(buffer, 256);
        readn = read(router_fd, buffer, 256);
        if ( readn < 0 ) {
            LOG("Unable to Read from Router");
            continue;
        }
        printf("PKT --> <%s>\n",buffer);

        if (strlen(buffer) == 0) {
            continue;
        }

        number_present = host_get_message_number(buffer);

        if (number_present && host_missing_packet(number_present)) {
            printf("Got Missing Sequence Packet number :%d\n",number_present);
        } else if (initial_step && number_prev - number_present  != 1 ) {
            /* Book Keep the Messages which are missing */
            host_missing_messages(number_prev, number_present);
            number_prev = number_present;
        } else {
            number_prev = number_present;
        }


        if (!initial_step) {
            /* Unset Initial Step as we got number_prev set in next step */
            initial_step = 1;
        }
    }

    return 0;
}
