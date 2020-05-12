//
//  main.c
//  DUMBserver
//
//  Created by Elaina Heraty on 11/30/19.
//  Copyright © 2019 Elaina Heraty. All rights reserved.
//

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>

int main(int argc, const char * argv[]) {
    int socket_desc;
    struct sockaddr_in server, client;
    
    //create socket
    socket_desc = socket(AF_INET, SOCK_STREAM, 0);
    if(socket_desc == -1){
        printf("Could not create socket\n");
    }
    
    //prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(12306); //listen on post 12306
    
    //bind
    if(bind(socket_desc, (struct sockaddr *)&server, sizeof(server)) == -1){
        printf("Failed to bind: error = %d\n", errno);
    }
    
    //start listening for connections
    listen(socket_desc, 3);
    
    //accept incoming connection
    int len = sizeof(struct sockaddr_in);
    int client_socket = accept(socket_desc, (struct sockaddr *)&client, (socklen_t *)&len);
    
    if(client_socket == -1){
        printf("Failed to accept connection: error = %d\n", errno);
    }
    return 0;
}
