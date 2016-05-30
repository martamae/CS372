/**********************************************************
* Name: Marta Wegner
* CS 372
* Program #2
*
* Description: Creates socket on SERVER_PORT and waits on 
* PORT_NUM for client to connect. 
* Establishes control connection w/ client.
* Waits for client to send a command over control connection
* 
* If invalid command sent - error msg sent
*
* Initiates data connection w/ client on DATA_PORT 
*
* Sends file or directory contents back to client
* if error w/ file send msg to client
*
* References:
*	http://stackoverflow.com/questions/4204666/how-to-list-
*	files-in-a-directory-in-a-c-program
**********************************************************/
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<sys/wait.h>
#include<fcntl.h>
#include<netinet/in.h>
#include<dirent.h>
#include<netdb.h>

/**********************************************************
*					startup()
*	Starts up the socket on serverPort - control connection
*	creates,bind, and listens for connections
*	
*	Parameters:
*		serverPort: port to start control connection on
*
*	Returns:
*		returns -1 on failed connection
*		returns controlSocket on successful connection
**********************************************************/
int startup (int serverPort) {
	int controlSocket;

	//Control connection
	//Create socket for control connection
	if((controlSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {//Create
		//If error creating print msg
		fprintf(stderr, "ERROR: server socket creation\n");
		return -1;
	}

	//Fill addr struct
	struct sockaddr_in server;

	server.sin_family = AF_INET;
	server.sin_port = htons(serverPort);
	server.sin_addr.s_addr = INADDR_ANY;

	//Bind socket to port
	if(bind(controlSocket, (struct sockaddr *) &server, sizeof(server)) == -1) {//bind
		//If bind error print msg
		fprintf(stderr, "ERROR: server bind call failed\n");
		return -1;
	}

	//Listen for connections
	if (listen(controlSocket, 5) == -1) { //listen
		//If listen call error print msg
		fprintf(stderr, "ERROR: server listen call failed\n");
		return -1;
	}

	return controlSocket;
}

int handleRequest(int clientControlSocket) {
	char reqType[2];
	int requestType;
	char data[10];
	int dataPort;
	char clientHostName[255];
	struct hostent * clientHost;
	char filename[255];
	char *ACK = "ACK";
	char rcvACK[4];

	//Receive request type from client
	if((recv(clientControlSocket, &reqType, sizeof(reqType), 0)) == -1) {
		//Error receiving requestType
		fprintf(stderr, "ERROR: error receiving request type from client\n\n", reqType);
		return -1;
	}
	
	//acknowledge
	if ((send(clientControlSocket, ACK, sizeof(ACK), 0)) == -1){
		//error Acknowledging
		fprintf(stderr, "ERROR: error acking request type");
	}

	//convert request type to int
	requestType = atoi(reqType);

	//Receive data port from client
	if((recv(clientControlSocket, &data, sizeof(data), 0)) == -1) {
		//Error receiving dataPort
		fprintf(stderr, "ERROR: error receiving data port from client\n\n");
		return -1;
	}
	
	//acknowledge
	if ((send(clientControlSocket, ACK, sizeof(ACK), 0)) == -1){
		//error Acknowledging
		fprintf(stderr, "ERROR: error acking data port");
	}
	
	//convert data port
	dataPort = atoi(data);

	//Receive hostname from client
	if((recv(clientControlSocket, &clientHostName, sizeof(clientHostName), 0)) == -1) {
		//Error receiving client host
		fprintf(stderr, "ERROR: error receiving client host\n\n");
		return -1;
	}
	
	//acknowledge
	if ((send(clientControlSocket, ACK, sizeof(ACK), 0)) == -1){
		//error Acknowledging
		fprintf(stderr, "ERROR: error acking data port");
	}

	//get host from hostname
	clientHost = gethostbyname(clientHostName);
	
	if (requestType == 1) { //if file request
		//Receiving the file name
		if((recv(clientControlSocket, &filename, sizeof(filename), 0)) == -1) {
			//Error receiving filename
			fprintf(stderr, "ERROR: error receiving filename from client\n\n");
			return -1;
		}
		
		//acknowledge
		if ((send(clientControlSocket, ACK, sizeof(ACK), 0)) == -1){
			//error Acknowledging
			fprintf(stderr, "ERROR: error acking data port");
		}
	}
	
	//set up data socket
	//create socket
	int dataSocket;

	if((dataSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1) { //create
		//If error creating
		fprintf(stderr, "ERROR: error creating data socket\n\n");
		return -1;
	}

	//set up address
	struct sockaddr_in client;

	//clear socket structure
	memset((char *)&client, 0, sizeof(client));

	client.sin_family = AF_INET;
	client.sin_port = htons(dataPort);
	memcpy(&client.sin_addr, clientHost->h_addr, clientHost->h_length);

	//Connect socket
	if(connect(dataSocket, (struct sockaddr*) &client, sizeof(client)) == -1) {
		fprintf(stderr, "ERROR: error connecting data port\n\n");
		return -1;
	}

	if(requestType == 1) {//if file request
		//Print msg to user
		printf("File '%s' requested on port %d.\n", filename, dataPort);

		//send file
		//open file for reading
		int file;
		if((file = open(filename, O_RDONLY)) == -1) {
			//error opening file
			fprintf(stderr, "ERROR: error opening file\n\n");
			
			char error[3];
			strncpy(error, "-1", 3);
			//send error signal to client
			send(dataSocket, &error, 2, 0);

			return -1;
		}

		//Get size of file
		int fileLength = lseek(file, 0, SEEK_END);

		//Convert size to strinf
		char lenStr[15];

		//fill lenStr w/ null chars
		memset((char *)&lenStr, '\0', sizeof(lenStr));

		//convert length to string
		sprintf(lenStr, "%d", fileLength);

		//send file text size
		if (send(dataSocket, &lenStr, sizeof(lenStr), 0) == -1) { //send
			//If error sending length
			fprintf(stderr, "ERROR: error sending file length\n\n");
			return -1;
		}

		//receive ACK
		recv(dataSocket, &rcvACK, sizeof(rcvACK), 0);

		//Create string to hold file
		char *fileText = malloc(sizeof(char) * fileLength);

		//Set file pointer to beginning of file
		lseek(file, 0, SEEK_SET);

		//Read file text into string
		if(read(file, fileText, fileLength) == -1) { //read
			//If error reading
			fprintf(stderr, "ERROR: error reading cipher text\n\n");
			return -1;
		}

		//send file
		int len = 0; //length sent
		while (len < fileLength) { //while whole file hasn't
								   //been sent
			char fileSend[1024]; //hold subset

			//copy subset of string to send
			strncpy(fileSend, &fileText[len], 1024);

			//send
			if (send(dataSocket, fileSend, 1024, 0) == -1) {
				//If error sending
				fprintf(stderr, "ERROR: error sending file\n\n");
				return -1;
			}

			len += 1024; //add length sent to len
		}
	
		//receive ACK
		recv(dataSocket, &rcvACK, sizeof(rcvACK), 0);
		
		//Print success message


		free(fileText);
	}
	else if(requestType == 0) {//if directory request
		//Print msg to user
		printf("List directory requested on port %d\n", dataPort);

		//send directory
		//open directory for reading
		DIR *dir;
		if((dir = opendir(".")) == NULL) {
			//error opening directory
			fprintf(stderr, "ERROR: error opening directory\n\n");
			
			//send error signal to client
			send(dataSocket, "-1", 2, 0);

			return -1;
		}

		//read from directory and send
		//send directory
		struct dirent *d;
		while ((d = readdir(dir)) != NULL) { //while whole file hasn't
											//been sent
			char name[40];

			//fill name w/ null chars
			memset((char *)&name, '\0', sizeof(name));

			//copy name to name variable
			strncpy(name, d->d_name, strlen(d->d_name));

			//if name == . or name == .. ignore
			if (strncmp(name, ".", sizeof(name)) == 0 || strncmp(name, "..", sizeof(name)) == 0){
				//do nothing
			}
			else { //else send
				//send
				if (send(dataSocket, name, sizeof(name), 0) == -1) {
					//If error sending
					fprintf(stderr, "ERROR: error sending directory name\n\n");
					return -1;
				}

				//receive ACK
				recv(dataSocket, &rcvACK, sizeof(rcvACK), 0);
			}
		}

		//close directory
		closedir(dir);
	}

	close(dataSocket);
	return 0;
}

/**********************************************************
*						main()
**********************************************************/
int main(int argc, char ** argv) {
	int serverPort;
	int controlSocket;
	int clientControlSocket;

	//if port number not specified
	if (argc != 2) {
		//Print error
		fprintf(stderr, "ERROR: You must include a port number\n\n");
		exit(1);
	}
	else {
		//get SERVER_PORT from args
		serverPort = atoi(argv[1]);
	}

	//start up
	if ((controlSocket = startup(serverPort)) == -1) {
		//Error in start up
		exit(1);
	}

	//Print msg that server is open
	printf("Server open on %d\n", serverPort);

	//loop and accept
	while (1) { //loop
		//accept
		clientControlSocket = accept(controlSocket, NULL, NULL);
		if (clientControlSocket == -1) {
			//If accept fails print msg
			fprintf(stderr, "ERROR: Accept call failed\n");
			exit(1);
		}

		int request;
		request = handleRequest(clientControlSocket);

		if (request == -1) {
			//Error receiving or connecting	
		}
		else if (request == -2) { 
			//Error opening or sending file
		}

		close(clientControlSocket);
	}

	close(controlSocket);

	return 0;
}