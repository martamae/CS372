# Name: Marta Wegner
# CS 372
# Program #2
#
# Description:
# 
# References:
# http://stackoverflow.com/questions/5574702/how-to-print-to-stderr-in-python
# http://www.tutorialspoint.com/python/python_networking.htm
# http://www.tutorialspoint.com/python/os_listdir.htm
# http://www.tutorialspoint.com/python/python_files_io.htm
# http://www.tutorialspoint.com/python/python_command_line_arguments.htm

#!/bin/python

import socket
import sys
import os


# Function: initiateContact()
# Initiates contact with the server (sets up control connection)
#
# Parameters:
#	serverHost: host that the server is on
#	serverPort: port number to connect to the server on
#
# Returns: 
#	controlSocket on successful connection
#	-1 on failed connection
def initiateContact(serverHost, serverPort):
	#create socket
	controlSocket = socket.socket()
	if (controlSocket == -1):
		return -1

	#connect
	if ((controlSocket.connect((serverHost, serverPort))) == -1):
		return -1

	return controlSocket


# Function: makeRequest()
# Makes request to server for list of directories or file
# over the control connection
# -l command makes request for list
# -g command makes request for file
# 
# Parameters:
#	request type
#		0 for -l command
#		1 for -g command
#	filename
#		filename of file to request from server
#	dataPort
#		port number that server should establish the data
#		connection on
#
# Returns:
#	0 on succesful request
#	-1 on failed request
def makeRequest(controlSocket, requestType, dataPort, filename):
	ACK = 0
	if (requestType == 0): #request directory list
		#send request type
		if (controlSocket.send(str(requestType)) == -1):
			#error sending requestType
			print >> sys.stderr, 'ERROR: error sending requestType\n\n'
			return -1

		#receive acknowledgement
		ACK = controlSocket.recv(4)

		#send dataPort
		dataPort = dataPort + '\0' #null terminate
		if (controlSocket.send(dataPort) == -1):
			#error sending dataPort
			print >> sys.stderr, 'ERROR: error sending dataPort\n\n'
			return -1

		#receive acknowledgement
		ACK = controlSocket.recv(4)

		#send client host
		#get hostname
		clientHost = socket.gethostname()

		if(controlSocket.send(clientHost) == -1):
			#error sending hostname
			print >> sys.stderr, 'ERROR: error sending hostname\n\n'
			return -1

		#receive acknowledgement
		ACK = controlSocket.recv(4)
	elif (requestType == 1): #request file
		#send request type
		if (controlSocket.send(str(requestType)) == -1):
			#error sending request type
			print >> sys.stderr, 'ERROR: error sending requestType\n\n'
			return -1

		#receive acknowledgement
		ACK = controlSocket.recv(4)

		#send dataPort
		dataPort = dataPort + '\0' #null terminate
		if (controlSocket.send(dataPort) == -1):
			#error sending dataPort
			print >> sys.stderr, 'ERROR: error sending dataPort\n\n'
			return -1

		#receive acknowledgement
		ACK = controlSocket.recv(4)

		#send client host
		#get hostname
		clientHost = socket.gethostname()

		if(controlSocket.send(clientHost) == -1):
			#error sending hostname
			print >> sys.stderr, 'ERROR: error sending hostname\n\n'
			return -1

		#receive acknowledgement
		ACK = controlSocket.recv(4)

		#null terminate filename to send
		filename = filename + '\0'
		
		#send filename
		if (controlSocket.send(filename) == -1):
			#error sending filename
			print >> sys.stderr, 'ERROR: error sending filename\n\n'
			return -1
		
		#receive acknowledgement
		ACK = controlSocket.recv(4)
	else:
		#invalid request type
		print >> sys.stderr, 'ERROR: error making request to server'
		return -1

	return 0

# receiveFile()
# Receives file from the server on the dataPort
#
# Parameter:
#		serverDataSocket: socket to receive file over
#		requestType: type of request
#			0 for directory request
#			1 for file request
#		filename: 
#			name of file to receive or -1 when 
#			directory is being received
#
# Returns:
#	-1 for failure
#	0 for success
def receiveFile(serverDataSocket, requestType, filename):
	if (requestType == 0): #directory
		#while there are still file names from the directory to receive
		
		r = 1
		print '\nDirectory Listing:'
		while(r): #while there is more info to receive
			#receive
			r = serverDataSocket.recv(40)

			if (r == '-1'):
				#error receiving directory
				print >> sys.stderr, 'ERROR: unable to receive directory\n\n'
				return -1
			elif (r == 0): 
				#if end of directory
				return 0
			else:
				#if directory file received received, print
				print '  ' + r

				#send ACK
				ACK = 'ACK'
				serverDataSocket.send(ACK)
	elif (requestType == 1): #file
		#receive length of file
		len = serverDataSocket.recv(15)

		print 'Length: ' + len

		#send ACK
		ACK = 'ACK'
		serverDataSocket.send(ACK)

		#if error receiving file
		fileLength = int(len.strip('\0'))
		
		#Error opening/sending file
		if (fileLength == "-1"):
			print >> sys.stderr, 'ERROR: file unable to be sent\n\n'
			return -1

		#compare name of file to files in directory
		dir = os.listdir('.')

		for file in dir:
			if (file == filename): 
				#if filename is already in directory rename
				pid = os.getpid() #get pid
				pid = str(pid) #convert to string
				filename = pid + filename

		# open file for appending
		file = open(filename, "a")

		#receive file
		len = 0 #length received
		while (len < fileLength): #while len received < total len expected
			#receive
			r = serverDataSocket.recv(1024)

			if (r == -1):
				#if error receiving file
				print >> sys.stderr, 'ERROR: error receiving file\n\n'
				return -1
			elif (r == 0):
				#if end of file
				return 0
			else:
				#add received portion to file
				file.write(r)

				len += 1024

		#send ACK
		ACK = 'ACK'
		serverDataSocket.send(ACK)
	
	return 0


# Function: main
def main(argv):
	#Validates command line parameters
	if (len(sys.argv) < 5):
		print >> sys.stderr, 'ERROR: Too few arguments included\n\n'
		sys.exit(1)
	
	#set up connection
	serverHost = argv[0]
	serverPort = int(argv[1])

	#initiate contact
	controlSocket = initiateContact(serverHost, serverPort)

	if(controlSocket == -1):
		#if error connecting to server
		print >> sys.stderr, 'ERROR: error connection to server\n\n'
		sys.exit(1)
	
	dataPort = 0
	requestType = -1
	filename = -1
	if (argv[2] == '-l'): #If list command
		if (len(sys.argv) != 5):
			#If the wrong number of arguments are included
			print >> sys.stderr, 'ERROR: ftclient.py <SERVER_HOST><SERVER_PORT> -l <DATA_PORT>\n\n'
			sys.exit(1)
	
		#requestType
		requestType = 0;

		#get dataPort
		dataPort = argv[3]

		#Ask for directory list
		makeRequest(controlSocket, requestType, dataPort, -1)
	elif (argv[2] == '-g'): #If get command
		if (len(sys.argv) != 6):
			#If the wrong number of args included
			print >> sys.stderr, 'ERROR: ftclient.py<SERVER_HOST> <SERVER _PORT> -g <FILENAME> <DATA_PORT>\n\n'
		
		#requestType
		requestType = 1

		#get filename
		filename = argv[3]

		#get dataPort
		dataPort = argv[4]

		#Ask for file
		makeRequest(controlSocket, requestType, dataPort, filename)
	else: #else invalid command
		#print error msg
		print >> sys.stderr, 'ERROR: invalid command\n\n'
		sys.exit(1)

	#set up data connection
	#create
	dataSocket = socket.socket()
	#error check
	if (dataSocket == -1):
		print >> sys.stderr, 'ERROR: error creating data socket\n\n'
		exit(1)

	#convert data port to int
	dataPort = int(dataPort)

	#bind
	x = dataSocket.bind(('0.0.0.0', dataPort))
	#error check
	if(x == -1):
		print >> sys.stderr, 'ERROR: error binding data socket\n\n'
		exit(1)

	#listen
	x = dataSocket.listen(5)
	#error check
	if (x == -1):
		print >> sys.stderr, 'ERROR: listening failed for data socket\n\n'
		exit(1)

	#accept
	serverDataSocket, addr = dataSocket.accept()
	#error check
	if(serverDataSocket == -1):
		print >> sys.stderr, 'ERROR: error accepteding data socket\n\n'

	receiveFile(serverDataSocket, requestType, filename)

	serverDataSocket.close

	dataSocket.close

	controlSocket.close

	return 0

if __name__ == "__main__":
	main(sys.argv[1:])