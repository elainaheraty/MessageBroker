//DUMBclient.c

#include <stdio.h>
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <string.h>
#include <unistd.h> 
#include <stdlib.h>
#include <ctype.h>
#include <stddef.h>
// Server Commands
/*	E.0	HELLO
	E.1	GDBYE
	E.2	CREAT
	E.3	OPNBX
	E.4	NXTMG
	E.5	PUTMG
	E.6	DELBX
	E.7	CLSBX */

// Client commands
/*  quit		(which causes: E.1 GDBYE)
	create		(which causes: E.2 CREAT)
	delete		(which causes: E.6 DELBX)			
	open		(which causes: E.3 OPNBX)
	close		(which causes: E.7 CLSBX)
	next		(which causes: E.4 NXTMG)
	put			(which causes: E.5 PUTMG) */


char* concat(const char *s1, const char *s2){
    char *result = malloc(strlen(s1) + strlen(s2) + 1); // +1 for the null-terminator
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}

void listCommands(){
	printf("Client commands and their respective server responses listeds below: \n");
	printf("quit		(which causes: E.1 GDBYE)\n");
	printf("create		(which causes: E.2 CREAT)\n");
	printf("delete		(which causes: E.6 DELBX)\n");
	printf("open		(which causes: E.3 OPNBX)\n");
	printf("close		(which causes: E.7 CLSBX)\n");
	printf("next		(which causes: E.4 NXTMG)\n");
	printf("put			(which causes: E.5 PUTMG)\n");
}

int main(int argc, char *argv[]){
	// Define variables
	char msgBuff[2000] = {0}; 
	int sock = 0, valread = 0;
	int msgBoxOpen = 0;
	char *prefix;
	char *suffix;
	char *both;
	char *msgBoxName;

	// Define port
	int PORT; 
    sscanf(argv[2], "%d", &PORT); 

	// Check if port is valid
	if(PORT < 4096 || PORT > 65535){
		printf("Port %d is not in the valid range, valid range is 4096 to 65535.\n", PORT);	
		return -1;
	}

	// Display entered information	
	printf("Attemtping to connect to %s", argv[1]);
	printf(": %s\n", argv[2]);

	// Create socket
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){ 
        printf("Unable to create socket\n"); 
        return -1; 
    } 

	// Setup connection
    struct sockaddr_in serv_addr; 
	serv_addr.sin_family = AF_INET; 
    serv_addr.sin_port = htons(PORT); 

	// Convert IP from string to binary 
    if(inet_pton(AF_INET, argv[1], &serv_addr.sin_addr)<=0){ 
        printf("\nInvalid address/ Address not supported \n"); 
        return -1; 
    }

	// Attempt connection three times
	int cnt = 0;
    while (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0){ 
        printf("Connection Failed \n"); 
		cnt++;
		if(cnt == 3){
		//	printf("Connection Attempts Terminated\n");
       		return -1;
		}
 
    } 
	printf("Connection successful\n");

	// Send HELLO (E.0) Message
	send(sock, "HELLO", 5, 0); 
  //  printf("Hello message sent\n");
    valread = read(sock, msgBuff, sizeof(msgBuff)); 
    msgBuff[valread] = '\0'; 
    printf("%s\n", msgBuff);
    if(strncmp(msgBuff, "HELLO DUMBv0 ready!", 19) != 0){
        printf("Bad server reponse \"%s\"\n", msgBuff);
    }

	msgBoxName = (char*)malloc(40);

	printf("Enter a command\n");
	// Get user input, loop until quit is entered
	while(1){
		// Malloc char arrays
		prefix = (char*)malloc(100);
		suffix = (char*)malloc(100);
		both = (char*)malloc(200);

		// Scans user input
		printf("> ");
		fgets(prefix, 100, stdin);

		// Removes newline (\n) from entered string
		strtok(prefix, "\n");

		if (strcmp(prefix, "create") == 0){
			// Replace prefix with server command
			prefix = "CREAT ";
			
			// Get name of box
			printf("Okay, enter a name for the new message box.\n");
			printf("create:> ");
			fgets(suffix, 100, stdin);

			// Check size
			char *e;
			int index;
			e = strchr(suffix, '\n');
			index = (int)(e - suffix);
			if(index < 5 || index > 25){
				printf("Name of message box not between 5-25 characters");
				break; // return -1;
			}

			// Removes newline (\n) from entered string
			char *p = strtok(suffix, "\n");
	
			// Check if first char is alphabetic
	/*		if(!isalpha(suffix[0])) {
				printf("First character is not alphabetic\n");			
				break;
				// ? return -1;
			}*/

			// Concatonate strings
			both = concat(prefix, suffix);
			
			// Send to server
			send(sock, both, strlen(both), 0); 

			// Get response
			valread = read(sock, msgBuff, sizeof(msgBuff));
			msgBuff[valread] = '\0';  
	
			// Check response
			if(strcmp(msgBuff, "OK!") == 0) 
				printf("Success! Message box '%s' has been created.\n", suffix);		
			else if(strcmp(msgBuff, "ER:EXIST") == 0) 
				printf("Error: Message box '%s' already exists.\n", suffix);
				
			else if(strcmp(msgBuff, "ER:WHAT?") == 0) 
				printf("Error.  Command was not understood.\n");
			else 
				printf("Operation failed.\n");
		} else if (strcmp(prefix, "delete") == 0){
			// Replace prefix with server command
			prefix = "DELBX ";

			// Get name of box
			printf("Okay, enter the name of the message box to delete.\n");
			printf("delete:> ");
			fgets(suffix, 100, stdin);

			// Removes newline (\n) from entered string
			strtok(suffix, "\n");

			// Concatonate strings
			both = concat(prefix, suffix);

			// Send to server
			send(sock, both, strlen(both), 0);
 
			// Get response
			valread = read(sock, msgBuff, sizeof(msgBuff));
			msgBuff[valread] = '\0';  

			// Check response
			if(strcmp(msgBuff, "OK!") == 0) 
				printf("Success! Message box '%s' has been deleted.\n", suffix);
			else if(strcmp(msgBuff, "ER:NEXST") == 0) 
				printf("Error.  Message box '%s' does not exist.\n", suffix);
			else if(strcmp(msgBuff, "ER:OPEND") == 0) 
				printf("Error.  Message box '%s' is currently open.\n", suffix);
			else if(strcmp(msgBuff, "ER:NOTMT") == 0) 
				printf("Error.  Message box '%s' still has messages.\n", suffix);
			else if(strcmp(msgBuff, "ER:WHAT?") == 0) 
				printf("Error.  Command was not understood.\n");
			else 
				printf("Operation failed.\n");
		} else if (strcmp(prefix, "open") == 0){
			// Replace prefix with server command
			prefix = "OPNBX ";

			// Get name of box
			printf("Okay, open which message box?\n");
			printf("open:> ");
			fgets(suffix, 100, stdin);

			// Removes newline (\n) from entered string
			strtok(suffix, "\n");

			// Concatone strings
			both = concat(prefix, suffix);

			// Send to server
			send(sock, both, strlen(both), 0);
 
			// Get response
			valread = read(sock, msgBuff, sizeof(msgBuff));
			msgBuff[valread] = '\0';  

			// Check response
			if(strcmp(msgBuff, "OK!") == 0) {
				printf("Success! Message box '%s' is now open.\n", suffix);
				
				// Copy name of string to another string for use later
				strcpy(msgBoxName, suffix);
			} else if(strcmp(msgBuff, "ER:NEXST") == 0) 
				printf("Error. Message box '%s' does not exist.\n", suffix);
			else if(strcmp(msgBuff, "ER:OPEND") == 0) 
				printf("Error. Message box '%s' is already open.\n", suffix);
			else if(strcmp(msgBuff, "ER:WHAT?") == 0) 
				printf("Error. Command was not understood.\n");
			else if(strcmp(msgBuff, "Got ERROR") == 0) 
				printf("Got ERROR\n");
			else
				printf("Operation failed.\n");
			msgBoxOpen = 1;
		} else if (strcmp(prefix, "close") == 0){
			//if(msgBoxOpen == 1){
				prefix = "CLSBX ";

				// Get name of box
				printf("Okay, close which message box?\n");
				printf("close:> ");
				fgets(suffix, 100, stdin);

				// Removes newline (\n) from entered string
				strtok(suffix, "\n");

				both = concat(prefix, suffix);

				// Send to server
				send(sock, both, strlen(both), 0);
 
				// Get response
				valread = read(sock, msgBuff, sizeof(msgBuff)); 
				msgBuff[valread] = '\0'; 

				// Check response
				if(strcmp(msgBuff, "OK!") == 0){
					printf("Message box '%s' successfully closed.\n", suffix);
					msgBoxOpen = 0;
				} else if(strcmp(msgBuff, "ER:WHAT?") == 0){
					printf("Error. Command was not understood.\n");
				} else if(strcmp(msgBuff, "ER:NOOPN") == 0){
					printf("Error. The message box is not open.\n");
				} else {
					printf("Operation failed.\n");
				}
		//	} else {
			//	printf("ER:NOOPN\n");			
			//}
		} else if (strcmp(prefix, "next") == 0){
		//	if(msgBoxOpen == 1){
				// Send to server
				send(sock, "NXTMG", 5, 0);
 
				// Get response
				valread = read(sock, msgBuff, sizeof(msgBuff)); 
				msgBuff[valread] = '\0'; 

				// Check response
				// Checks if "OK!" is in the buffer
				if(strstr(msgBuff, "OK!") != NULL){
					printf(msgBuff);
					printf("\n");
				} else if(strcmp(msgBuff, "ER:NOOPN") == 0){
					printf("Error: The message box is not open.\n");
				} else if(strcmp(msgBuff, "ER:EMPTY") == 0){
					printf("Error: Message box is empty or has no more entries.\n");
				} else if(strcmp(msgBuff, "ER:WHAT?") == 0){
					printf("Error.  Command was not understood.\n");
				} else {
					printf("Operation failed.\n");
				}
			//} else {
			//	printf("ER:NOOPN\n");			
			//}
		} else if (strcmp(prefix, "put") == 0){
		//	if(msgBoxOpen == 1){// && inpBuff[0] != '\0'){
				prefix = "PUTMG!";
				char* suffSize = (char*)malloc(2);

				// Get message for box				
				printf("Okay, what message do you want to put in there?\n");
				printf("put:> ");
				fgets(suffix, 100, stdin);

				// Removes newline (\n) from entered string
				strtok(suffix, "\n");

				// Get size of newly entered string
				size_t size = strlen(suffix);
				sprintf(suffSize, "%d", size);

				// Concatonate strings
				suffSize = concat(suffSize, "!"); // Add '!' to size
				both = concat(prefix, suffSize); // Add PUTMG! to size!
				both = concat(both, suffix); // Add PUTMG!size! to msg

				// Send to server
				send(sock, both, strlen(both), 0); 

				// Get response
				valread = read(sock, msgBuff, sizeof(msgBuff)); 
				msgBuff[valread] = '\0'; 

				// Check response
				if(strcmp(msgBuff, "OK!") == 0) 
					printf("Message successfully added to '%s'\n", msgBoxName);
				else if(strcmp(msgBuff, "ER:NOOPN") == 0) 
					printf("Error. No message box is open.\n");
				else if(strcmp(msgBuff, "ER:WHAT?") == 0) 
					printf("Error.  Command was not understood.\n");
				else
					printf("Operation failed.\n");
		//	} else {
		//		printf("No message box open");
		//	}
		} else if(strcmp(prefix, "help") == 0){
			listCommands();	
		} else {
			if(strcmp(prefix, "quit") != 0){
				printf("That is not a command, for a command list enter 'help'.\n");
			} else {
				// Send message
				send(sock, "GDBYE", 5, 0); 
				// Get response
		//		valread = read(sock, msgBuff, sizeof(msgBuff)); 
		//		msgBuff[valread] = '\0';

				// If server response isn't empty... 
		//		if(msgBuff != "")
		//			break;
		//		else
		//			printf("Server responded with error, connection still open\n");
				break;
			}
		}
	}

	// Close connection
	close(valread);
	close(sock);
	return 0;
}
