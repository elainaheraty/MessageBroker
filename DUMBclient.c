//
//  main.c
//  DUMBclient
//
//  Created by Elaina Heraty on 11/30/19.
//  Copyright © 2019 Elaina Heraty. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdbool.h>

bool isValidIpAddress(const char *ipAddress)
{
    struct sockaddr_in sa;
    int result = inet_pton(AF_INET, ipAddress, &(sa.sin_addr));
    return result != 0;
}

int main(int argc, const char * argv[]) {
    int socket_desc;
    struct sockaddr_in server;
    char* message;
    
    if(argc < 3){
        printf("too few arguments\n");
        return -1;
    }
    
    if(!isValidIpAddress(argv[1])){
        printf("invalid ip address\n");
        return -1;
    }
    int port_num = atoi(argv[2]);
    if(!(port_num >= 10000 && port_num <= 65536)){ //not within port # range
        printf("invalid port number, not within range of [10,000, 65,536]\n");
        return -1;
    }
 
	

   
    printf("ipaddress = %s\nport number = %d\n", argv[1], port_num);
    
    //create socket
    socket_desc = socket(AF_INET , SOCK_STREAM , 0); //(int family, int type, int protocol)
    //AF_INET -> IPv4 protocols; SOCK_STREAM -> stream socket, 0 -> either TCP, UDP, SCTP
    
    if(socket_desc == -1){
        printf("Failed to create socket, errno=%d\n", errno);
        return -1;
    }
    
    server.sin_addr.s_addr = inet_addr(argv[1]); //sin_addr is the IP address in the socket, sin_addr is a union that can be accessed as s_addr (one 4-bytes integer)
    server.sin_family = AF_INET;
    server.sin_port = htons(port_num); // htons stores bytes in memory (byte representation of port #) in universal way, changes little endian to big endian
    
    //connect to remote server
    if(connect(socket_desc, (struct sockaddr *)&server, sizeof(server)) < 0){ //connect function takes in: socket descriptor returned by socket function, a pointer to struct sockaddr (contains destination IP address and po rt), size you want set to (suggested sizeof(struct sockaddr))
     
        if(errno == EINPROGRESS){
            fd_set wait_set;
        }
        printf("connect error, errno=%d\n", errno);
        return -1;
    }
    
    printf("Connection established.\n");
    
    //send some data
    message = "HELLO";

    char rx_msg[2000];
    char tx_msg[2000];
    int rx_size;    
    int tx_size;

    if(send(socket_desc, message, strlen(message), 0) < 0){ //send function takes in socket descriptor, pointer to data you want sent, length of data you want sent in bytes, flags(set to 0)
        printf("Send failed, errno=%d\n", errno);
        return -1;
    }

    while( (rx_size = (int)recv(socket_desc, rx_msg , 2000 , 0)) > -1 )
    {
	printf("Received data..");
        if(rx_size == 0){
            continue;
        }
        rx_msg[rx_size] = '\0';
       // if(processMessage(rx_msg, tx_msg) == 0){
         //   ret = send(sock , tx_msg , (size_t)strlen(tx_msg), 0);
           // if(ret == -1){
            //    break;
           // }
          //  printf("sent msg \"%s\" to client\n", tx_msg);
       // }
	printf("msg=%s", rx_msg);
    }
    
    printf("Data sent.\n");
        
}
