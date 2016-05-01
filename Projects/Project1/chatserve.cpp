/**********************************************************
 * Name: Marta Wegner
 * Project #1
 * File name: chatserve.cpp
 * Description: server side of chat program.
 * Starts on Host A
 * connects to chatclient.c
 *
 * Listens for connections
 * Connects socket to chatclient.c
 * takes turns sending msgs
 * "/quit" closes socket continues to 
 * wait for other connection
 *
 * reference: linuxhowtos.org
 *********************************************************/
#include<cstdio>
#include<cstdlib>
#include<unistd.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<sys/wait.h>
#include<fcntl.h>
#include<netinet/in.h>
#include<cstring>
#include<netdb.h>


/**********************************************************
 * 		       initiate
 * 	Parameters:
 * 	portNum: port to wait for connection
 *
 * 	Creates socket, binds, and listens
 *
 * 	return value:
 * 	-1 is failed to create, bind, or listen
 * 	else returns socketfd
 *********************************************************/
int initiate (int portNum) {
     //create socket
    int socketfd;

    //create
    if((socketfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
	//If error creating
	fprintf(stderr, "Could not resolve host name\n");
	return -1;
    }

    struct sockaddr_in server;

    //clear socket structure
    memset((char *)&server, 0, sizeof(server));

    //fill socket struct
    server.sin_family = AF_INET;
    server.sin_port = htons(portNum);
    server.sin_addr.s_addr = INADDR_ANY;

    //bind socket to port
    if(bind(socketfd, (struct sockaddr *) &server, sizeof(server)) == -1) {
	//if bind error
	fprintf(stderr, "bind called failed\n");
	return -1;
    }

    //listen for connections
    if(listen(socketfd,5) == -1) {//listen
	//if listen call error
	fprintf(stderr,"listen call failed\n");
	return -1;
    }

    return socketfd;
}

/**********************************************************
 * 			sendMsg
 * 	parameter:
 * 	socketfd: socket file descritor
 * 	handle: user's handle
 *
 * 	Send  msg to server
 *
 * 	return value:
 * 	-1 if error sending
 * 	-2 if user quits
 * 	0 is msg sent w/o error
 * *******************************************************/
int sendMsg (int client_socket, char handle[12]) {
    char msg[511];
    char message[501];

    //clear msg and message
    memset((char *)msg, '\0', sizeof(msg));
    memset((char *)message, '\0', sizeof(message));
	
    strcpy(msg, handle); //handle into msg
	
    //Enter msg
    strcat(msg, "> ");
    printf("%s",msg);//prompt
    fgets(message,501,stdin);//input

    //if \quit is entered
    //close connection
    if (strncmp(message, "\\quit", 4) == 0) {
	strcpy(msg, "-1");//quit msg to send

	//send quit message
	if(send(client_socket, &msg, sizeof(msg), 0) == -1) {
	    //error sending msg
	    fprintf(stderr, "Error sending msg from client.\n");
	    return -1;
	}

	return -2;
    }
    else { //else send message
	//Add handle and message
	strcat(msg,message);

	//send message
	if(send(client_socket, &msg, sizeof(msg), 0) == -1) {
	    //error sending msg
	    fprintf(stderr, "Error sending msg from client.\n");
	    return -1;
	}
    }

    return 0;
}


/**********************************************************
 * 			recvMsg
 * 	parameter:
 * 	socketfd: socket file descritor
 *
 * 	Receives and prints msg from server
 *
 * 	return value:
 * 	-1 if error receving
 * 	-2 if server ended session
 * 	0 is msg received and printed
 * *******************************************************/
int recvMsg(int client_socket) {
    char msg[511];

    //clear msg
    memset((char *)msg, '\0', sizeof(msg));

    if(recv(client_socket, &msg, sizeof(msg), 0) == -1) {
	//Error receiving msg
	fprintf(stderr,"Error receiving msg from server.\n");
	return -1;
    }
    else if (strncmp(msg,"-1",2) != 0) {
 	//else print received msg
	printf("%s", msg);
    }
    else {
	//Else print quit msg
	printf("Server has ended the chat session.\n");
	return -2;
    }

    return 0;
}

/**********************************************************
 * 			main
 *********************************************************/
int main(int argc, char ** argv) {
    char  handle[7] = "Host A";
    int i;
    int portNum;

    handle[7] = '\0'; //Null terminate

   //Get server port number
   //check that right num of args
    if (argc < 2) {
	//Print error	
	fprintf(stderr, "You must include port number.\n");
	exit(1);
    }
    else {
	//get server port number
	portNum = atoi(argv[1]);
    }

    //Server
    int socketfd; 

    //Start up connection and send initial msg
    if((socketfd = initiate(portNum)) == -1) {
	//creating, binding, or listening
	exit(1);
    }

    int client_socket;

    //loop and accept
    while(1) {//loop
	//Wait msg
	printf("Waiting for connections...\n");

        //accept
        client_socket = accept(socketfd, NULL, NULL);
	if (client_socket == -1) {
	    //if accept fails
	    fprintf(stderr, "accept call failed\n");
	    exit(1);
	}
 	
        int x = 0;
	//receive initial msg
	 if((x = recvMsg(client_socket)) == -1) {
    	    //if error receiving
    	    exit(1);
	 }

	
        //receive/send until quit msg is entered
    	while (x != -2) {
	    //take turns sending and receiving messages
	    
            //send
	    if((x = sendMsg(client_socket, handle)) == -1) {
	        //if error sending
	        exit(1);
	    }
	    else if(x == -2) {
	        //server ended chat session
	        printf("Chat session ended\n");
	        close(client_socket);
	    }
	    else { //msg sent, now receive
	        if((x = recvMsg(client_socket)) == -1) {
	    	    //if error receiving
	            exit(1);
	        }
	        else if(x == -2) {
	            //client ended chat session
	            printf("Client ended chat session\n");
	            close(client_socket);
	        }
    	    }
	}
    }
    
    return 0;
}
