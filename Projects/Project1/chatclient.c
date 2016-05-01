/**********************************************************
 * Name: Marta Wegner
 * Project #1
 * File name: chatclient.c
 * Description: client side of chat program.
 * Starts on Host B
 * connects to chatserve.cpp
 *
 * Gets users handle
 * Connects socket to chatserve.cpp
 * sends initial msg
 * takes turns sending msgs
 * "/quit" ends the progam and closes socket
 *
 * reference: linuxhowtos.org
 *********************************************************/
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<sys/wait.h>
#include<fcntl.h>
#include<netinet/in.h>
#include<string.h>
#include<netdb.h>


/**********************************************************
 * 		       Startup
 * 	Parameters:
 * 	host: server host
 * 	serverPort: port to connect to server on
 *
 * 	Creates socket and connects to server
 *
 * 	return value:
 * 	-1 is failed to create or connect
 * 	else returns socketfd
 *********************************************************/
int startup (struct hostent *host, int serverPort) {
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
    server.sin_port = htons(serverPort);
    bcopy((char *)host->h_addr, (char *)&server.sin_addr.s_addr,
	   host->h_length);

    //Connect socket
    if(connect(socketfd, (struct sockaddr*) &server, 
	sizeof(server)) == -1) {
	//connect error
	fprintf(stderr, "Connect error\n");
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
int sendMsg (int socketfd, char handle[12]) {
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
	if(send(socketfd, &msg, sizeof(msg), 0) == -1) {
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
	if(send(socketfd, &msg, sizeof(msg), 0) == -1) {
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
int recvMsg(int socketfd) {
    char msg[511];

    //clear msg
    memset((char *)msg, '\0', sizeof(msg));

    if(recv(socketfd, &msg, sizeof(msg), 0) == -1) {
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
    char  handle[12];
    int i;
    int ch;
    int oneWord = 1;
    int tenChar = 0;
    int serverPort;
    struct hostent *host;

   //Get server port number and hostname
   //check that right num of args
    if (argc < 3) {
	//Print error	
	fprintf(stderr, "You must include port number and hostname.\n");
	exit(1);
    }
    else {
	//get server port number
	serverPort = atoi(argv[2]);
	//get server hostname
	host = gethostbyname(argv[1]);
    }

    if(host == NULL) {
	//If server does not exist
	fprintf(stderr,"No such host.\n");
	exit(1);
    }

    memset(handle, '\0', 12); //set handle to \0
			      //to null terminate


    //Get clients handle
    printf("What is your handle?\n");//promt
    fgets(handle,12,stdin);//input   

    //check that handle is entered correctly
    while (oneWord == 0 || tenChar == 0) {
 	//check length
    	if (handle[10] != '\n' && handle[10] != '\0') {
	    //clear buffer
	    while((ch = getchar()) != '\n' &&ch != EOF);

            memset(handle, '\0', 12); //set handle to \0

	    //Error msg
	    fprintf(stderr, "Enter handle that is 10 char or fewer:\n");
	    fgets(handle,12,stdin);//new input
	    tenChar = 0;//set bool = false
    	}
	else {
	    tenChar = 1;//set bool = true
	}

	oneWord = 1;
   	//Check that is one word
      	for(i = 0; i < 10; i++) {
	    if(handle[i] == ' ') {
            	memset(handle, '\0', 12); //set handle to \0

		//Error msg
  	    	fprintf(stderr, "Enter handle that is one word:\n");
	    	fgets(handle,12,stdin);//new input
		oneWord = 0;
	    }
	    else if(handle[i] == '\n') {
		handle[i] = '\0';//remove newline
	    }
   	}

	//Null terminate
	handle[11] = '\0';
	handle[10] = '\0';
    } 


    //Client
    int socketfd; 

    //Start up connection and send initial msg
    if((socketfd = startup(host, serverPort)) == -1) {
	//If error starting up connection
	//or sending initial msg
	exit(1);
    }


    //receive/send until quit msg is entered
    int x = 0;
    //send initial msg
    if((x = sendMsg(socketfd, handle)) == -1) {
	//if error sending
	exit(1);
    }
    else if(x == -2) {
	//server ended chat session
	printf("Chat session ended\n");
	close(socketfd);
	exit(0);
    }

    while (x != -2) {
	//take turns sending and receiving messages
	
	//receive msg
	if((x = recvMsg(socketfd)) == -1) {
	    //if error receiving
	    exit(1);
	}
	else if(x == -2) {
	    //server ended chat session
	    printf("Chat session ended by server\n");
	    close(socketfd);
	    exit(0);
	}

	//send
	if((x = sendMsg(socketfd, handle)) == -1) {
	    //if error sending
	    exit(1);
	}
	else if(x == -2) {
	    //server ended chat session
	    printf("Chat session ended\n");
	    close(socketfd);
	    exit(0);
	}
    } 
    
    return 0;
}
