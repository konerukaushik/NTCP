 
#include <stdio.h>
#include <string.h> 
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>  
#include <arpa/inet.h>  
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h> 
#include "router.h"

int  pkt_count = 0; 
int main( )
{
    int                 opt                 = TRUE;
    int                 master_socket       = 0; 
    int                 addrlen             = 0;
    int                 new_socket          = 0;
    int                 client_socket[3];
    int                 socket_fd_packets[3][2];
    int                 max_clients         = 3; 
    int                 activity, i , readn , sd;
    int                 max_sd;
    int                 rest_client, iter_run;
    struct sockaddr_in  address;
    char                buffer[256];
    fd_set              readfds;
      
    for (i = 0; i < max_clients; i++) 
    {
        client_socket[i] = 0;
        socket_fd_packets[i][0] = 0;
        socket_fd_packets[i][1] = 0;
    }
      
    if( (master_socket = socket(AF_INET , SOCK_STREAM , 0)) == 0) 
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
  
    if( setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0 )
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
  
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons( ROUTER_PORT );
      
    if (bind(master_socket, (struct sockaddr *)&address, sizeof(address))<0) 
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    printf("Listening on port %d \n", ROUTER_PORT);
     
    if (listen(master_socket, 3) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }
      
    addrlen = sizeof(address);
    puts("Waiting for connections ...");
     
    while(TRUE) 
    {
        FD_ZERO(&readfds);
  
        FD_SET(master_socket, &readfds);
        max_sd = master_socket;
        for ( i = 0 ; i < max_clients ; i++) 
        {
            sd = client_socket[i];
            if(sd > 0)
                FD_SET( sd , &readfds);
             
            if(sd > max_sd)
                max_sd = sd;
        }
  
        activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);
    
        if ((activity < 0)) 
        {
            printf("select error");
        }
          
        if (FD_ISSET(master_socket, &readfds)) 
        {
            if ((new_socket = accept(master_socket, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)
            {
                perror("accept");
                exit(EXIT_FAILURE);
            }
          
            printf("New connection , socket fd is %d , ip is : %s , port : %d \n" , new_socket , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));
        
            for (i = 0; i < max_clients; i++) 
            {
                if( client_socket[i] == 0 )
                {
                    client_socket[i] = new_socket;
                    socket_fd_packets[i][0] = new_socket;
                    printf("Adding to list of sockets as %d\n" , i);
                    break;
                }
            }
        }
          
        //else its some IO operation on some other socket :)
        for (i = 0; i < max_clients; i++) 
        {
            sd = client_socket[i];
              
            if (FD_ISSET( sd , &readfds)) 
            {
                if ((readn = read( sd , buffer, 1024)) == 0)
                {
                    getpeername(sd , (struct sockaddr*)&address , (socklen_t*)&addrlen);
                    printf("Host disconnected , ip %s , port %d \n" , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));
                      
                    close( sd );
                    client_socket[i] = 0;
                    socket_fd_packets[i][1] = 0;
                    socket_fd_packets[i][0] = 0;
                } else {
                    socket_fd_packets[i][1]++;
                    
                    if (!strstr(buffer, "Missing") && PKT_TO_DROP && socket_fd_packets[i][1] % (PKT_TO_DROP) == 0) {
                        buffer[readn] = '\0';
                        printf("Dropping Packet %s ... \n",buffer);
                        break;
                    }
                    rest_client = 0;
                    iter_run    = 0;
                    if ((socket_fd_packets[i][1] % Y) == 0) {
                        printf("Delaying ....\n");
                        usleep(1000);
                    }

                    buffer[readn] = '\0';
                    while( iter_run < max_clients) { 
                            rest_client = client_socket[iter_run++];
                            if (rest_client != sd && rest_client != 0) 
                                send(rest_client , buffer , strlen(buffer) , 0 );
                    }
                }
            }
        }
        for (i =0;i<3;i++) 
            printf("Number of Packet Transmitted %d\n", socket_fd_packets[i][1]);
    }
      
    return 0;
} 
