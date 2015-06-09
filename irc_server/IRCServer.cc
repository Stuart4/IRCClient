
const char * usage =
"                                                               \n"
"IRCServer:                                                   \n"
"                                                               \n"
"Simple server program used to communicate multiple users       \n"
"                                                               \n"
"To use it in one window type:                                  \n"
"                                                               \n"
"   IRCServer <port>                                          \n"
"                                                               \n"
"Where 1024 < port < 65536.                                     \n"
"                                                               \n"
"In another window type:                                        \n"
"                                                               \n"
"   telnet <host> <port>                                        \n"
"                                                               \n"
"where <host> is the name of the machine where talk-server      \n"
"is running. <port> is the port number you used when you run    \n"
"daytime-server.                                                \n"
"                                                               \n";

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fstream>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <vector>
#include <string.h>
#include <stdio.h>
#include <iostream>
#include <time.h>

#include "IRCServer.h"
#include "IRCVector.h"
#include "HashTableVoid.cc"


using namespace std;
int QueueLength = 5;
HashTableVoid rooms;
HashTableVoid users;
HashTableVoid mess;

int
IRCServer::open_server_socket(int port) {

	// Set the IP address and port for this server
	struct sockaddr_in serverIPAddress; 
	memset( &serverIPAddress, 0, sizeof(serverIPAddress) );
	serverIPAddress.sin_family = AF_INET;
	serverIPAddress.sin_addr.s_addr = INADDR_ANY;
	serverIPAddress.sin_port = htons((u_short) port);

	// Allocate a socket
	int masterSocket =  socket(PF_INET, SOCK_STREAM, 0);
	if ( masterSocket < 0) {
		perror("socket");
		exit( -1 );
	}

	// Set socket options to reuse port. Otherwise we will
	// have to wait about 2 minutes before reusing the same port number
	int optval = 1; 
	int err = setsockopt(masterSocket, SOL_SOCKET, SO_REUSEADDR, 
			(char *) &optval, sizeof( int ) );

	// Bind the socket to the IP address and port
	int error = bind( masterSocket,
			(struct sockaddr *)&serverIPAddress,
			sizeof(serverIPAddress) );
	if ( error ) {
		perror("bind");
		exit( -1 );
	}

	// Put socket in listening mode and set the 
	// size of the queue of unprocessed connections
	error = listen( masterSocket, QueueLength);
	if ( error ) {
		perror("listen");
		exit( -1 );
	}

	return masterSocket;
}

	void
IRCServer::runServer(int port)
{
	int masterSocket = open_server_socket(port);

	initialize();

	while ( 1 ) {

		// Accept incoming connections
		struct sockaddr_in clientIPAddress;
		int alen = sizeof( clientIPAddress );
		int slaveSocket = accept( masterSocket,
				(struct sockaddr *)&clientIPAddress,
				(socklen_t*)&alen);

		if ( slaveSocket < 0 ) {
			perror( "accept" );
			exit( -1 );
		}

		// Process request.
		processRequest( slaveSocket );		
	}
}

	int
main( int argc, char ** argv )
{
	// Print usage if not enough arguments
	if ( argc < 2 ) {
		fprintf( stderr, "%s", usage );
		exit( -1 );
	}

	// Get the port from the arguments
	int port = atoi( argv[1] );

	IRCServer ircServer;

	// It will never return
	ircServer.runServer(port);

}

//
// Commands:
//   Commands are started y the client.
//
//   Request: ADD-USER <USER> <PASSWD>\r\n
//   Answer: OK\r\n or DENIED\r\n
//
//   REQUEST: GET-ALL-USERS <USER> <PASSWD>\r\n
//   Answer: USER1\r\n
//            USER2\r\n
//            ...
//            \r\n
//
//   REQUEST: CREATE-ROOM <USER> <PASSWD> <ROOM>\r\n
//   Answer: OK\n or DENIED\r\n
//
//   Request: LIST-ROOMS <USER> <PASSWD>\r\n
//   Answer: room1\r\n
//           room2\r\n
//           ...
//           \r\n
//
//   Request: ENTER-ROOM <USER> <PASSWD> <ROOM>\r\n
//   Answer: OK\n or DENIED\r\n
//
//   Request: LEAVE-ROOM <USER> <PASSWD>\r\n
//   Answer: OK\n or DENIED\r\n
//
//   Request: SEND-MESSAGE <USER> <PASSWD> <MESSAGE> <ROOM>\n
//   Answer: OK\n or DENIED\n
//
//   Request: GET-MESSAGES <USER> <PASSWD> <LAST-MESSAGE-NUM> <ROOM>\r\n
//   Answer: MSGNUM1 USER1 MESSAGE1\r\n
//           MSGNUM2 USER2 MESSAGE2\r\n
//           MSGNUM3 USER2 MESSAGE2\r\n
//           ...\r\n
//           \r\n
//
//    REQUEST: GET-USERS-IN-ROOM <USER> <PASSWD> <ROOM>\r\n
//    Answer: USER1\r\n
//            USER2\r\n
//            ...
//            \r\n
//

	void
IRCServer::processRequest( int fd )
{
	// Buffer used to store the comand received from the client
	const int MaxCommandLine = 1024;
	char commandLine[ MaxCommandLine + 1 ];
	int commandLineLength = 0;
	int n;

	// Currently character read
	unsigned char prevChar = 0;
	unsigned char newChar = 0;

	//
	// The client should send COMMAND-LINE\n
	// Read the name of the client character by character until a
	// \n is found.
	//

	// Read character by character until a \n is found or the command string is full.
	while ( commandLineLength < MaxCommandLine &&
			read( fd, &newChar, 1) > 0 ) {

		if (newChar == '\n' && prevChar == '\r') {
			break;
		}

		commandLine[ commandLineLength ] = newChar;
		commandLineLength++;

		prevChar = newChar;
	}

	// Add null character at the end of the string
	// Eliminate last \r
	commandLineLength--;
	commandLine[ commandLineLength ] = 0;

	printf("RECEIVED: %s\n", commandLine);

	/*printf("The commandLine has the following format:\n");
	printf("COMMAND <user> <password> <arguments>. See below.\n");
	printf("You need to separate the commandLine into those components\n");
	printf("For now, command, user, and password are hardwired.\n");
	*/

	const char * command = strtok(commandLine, " ");
	const char * user = strtok(NULL, " ");
	const char * password = strtok(NULL, " ");
	const char * args = strtok(NULL, "");

	if (command == NULL || user == NULL || password == NULL) {
		const char * msg =  "DENIED\r\n";
		write(fd, msg, strlen(msg));
		close(fd);
		return;
	}

	printf("command=%s\n", command);
	printf("user=%s\n", user);
	printf( "password=%s\n", password );
	printf("args=%s\n", args);

	if (!strcmp(command, "ADD-USER")) {
		addUser(fd, user, password, args);
	}
	else if (!strcmp(command, "CREATE-ROOM")) {
		createRoom(fd, user, password, args);
	}
	else if (!strcmp(command, "ENTER-ROOM")) {
		enterRoom(fd, user, password, args);
	}
	else if (!strcmp(command, "LEAVE-ROOM")) {
		leaveRoom(fd, user, password, args);
	}
	else if (!strcmp(command, "SEND-MESSAGE")) {
		sendMessage(fd, user, password, args);
	}
	else if (!strcmp(command, "GET-MESSAGES")) {
		getMessages(fd, user, password, args);
	}
	else if (!strcmp(command, "GET-USERS-IN-ROOM")) {
		getUsersInRoom(fd, user, password, args);
	}
	else if (!strcmp(command, "LIST-ROOMS")) {
		listRooms(fd, user, password, args);
	}
	else if (!strcmp(command, "GET-ALL-USERS")) {
		getAllUsers(fd, user, password, args);
	}
	else {
		const char * msg =  "UNKNOWN COMMAND\r\n";
		write(fd, msg, strlen(msg));
	}

	// Send OK answer
	//const char * msg =  "OK\n";
	//write(fd, msg, strlen(msg));

	close(fd);	
}

	void
IRCServer::initialize()
{
	// Open password file
	fstream fileStream;
	fileStream.open(PASSWORD_FILE, fstream::out | fstream::in | fstream::app);
	char line[500];
	while (fileStream.getline(line, 500)) {
		int i = 0;
		while (line[i] != ':') i++;
		line[i] = '\0';
		IRCVector * vect = new IRCVector();
		users.insertItem(strdup(line), vect);
	}
	fileStream.close();

	// Initialize users in room

	// Initalize message list

}

bool
IRCServer::checkPassword(int fd, const char * user, const char * password) {
	// Here check the password
	using namespace std;
	string lookFor(user);
	lookFor.append(":").append(password);
	fstream fileStream;
	fileStream.open(PASSWORD_FILE, fstream::out | fstream::in | fstream::app);
	char line[500];
	while (fileStream.getline(line, 500)) {
		string sline(line);
		if (lookFor.compare(sline) == 0) {
			fileStream.close();
			return true;
		}
	}
	fileStream.close();
	const char * msg =  "ERROR (Wrong password)\r\n";
	write(fd, msg, strlen(msg));
	return false;
}
bool
IRCServer::userExists(const char * user) {
	using namespace std;
	string lookFor(user);
	fstream fileStream;
	fileStream.open(PASSWORD_FILE, fstream::out | fstream::in | fstream::app);
	char line[500];
	while (fileStream.getline(line, 500)) {
		cout << line << endl;
		string sline(line);
		if (lookFor.compare(sline.substr(0, lookFor.length())) == 0) {
			fileStream.close();
			return true;
		}
	}
	fileStream.close();
	return false;
}

	void
IRCServer::addUser(int fd, const char * user, const char * password, const char * args)
{
	// Here add a new user. For now always return OK.
	using namespace std;
	if (userExists(user)) {
		if (!checkPassword(fd, user, password)) {
			return;
		}
		const char * msg =  "OK\r\n";
		write(fd, msg, strlen(msg));
		return;
	}
	fstream fileStream;
	fileStream.open(PASSWORD_FILE, fstream::out | fstream::in | fstream::app);
	fileStream << user << ":" << password << endl;
	fileStream.close();

	const char * msg =  "OK\r\n";
	write(fd, msg, strlen(msg));
	IRCVector * vect = new IRCVector();
	users.insertItem(strdup(user), vect);

	return;
}

bool 
IRCServer::userIsInRoom(const char * user, const char * args) {
	try {
		IRCVector * vect;
		if (user != NULL && args != NULL && rooms.find(args, (void **)&vect)) {
			return vect->find(user);
		}
	} catch (...) {
		return false;
	}
	return false;
}

bool 
IRCServer::roomExists(const char * args) {
	try {
		IRCVector * vect;
		return rooms.find(args, (void **) &vect);
	} catch (...) {
		return false;
	}
	return true;
}

	void
IRCServer::createRoom(int fd, const char * user, const char * password, const char * args)
{
	if (args == NULL) {
		const char * msg =  "DENIED\r\n";
		write(fd, msg, strlen(msg));
		return;
	}
	args = strtok(strdup(args), " ");
	if (!checkPassword(fd, user, password)) {
		return;
	}
	if (roomExists(args)) {
		const char * msg =  "DENIED\r\n";
		write(fd, msg, strlen(msg));
		return;
	}
	IRCVector * vect = new IRCVector();
	rooms.insertItem(strdup(args), vect);
	vect = new IRCVector();
	vect->limited = true;
	mess.insertItem(strdup(args), vect);
	const char * msg =  "OK\r\n";
	write(fd, msg, strlen(msg));
}

	void
IRCServer::enterRoom(int fd, const char * user, const char * password, const char * args)
{
	args = strtok(strdup(args), " ");
	if (!checkPassword(fd, user, password)) {
		return;
	}
	if (!roomExists(args)) {
		const char * msg =  "ERROR (No room)\r\n";
		write(fd, msg, strlen(msg));
		return;
	}
	IRCVector * vect;
	if (rooms.find(args, (void **)&vect)) {
		if (vect->find(user)) {
			const char * msg =  "OK\r\n";
			write(fd, msg, strlen(msg));
			return;
		}
	}
	users.find(user, (void **) &vect);
	string s(args);
	vect->insert(&s);
	rooms.find(args, (void **) &vect);
	string p(user);
	vect->insert(&p);
	const char * msg =  "OK\r\n";
	write(fd, msg, strlen(msg));
}

	void
IRCServer::leaveRoom(int fd, const char * user, const char * password, const char * args)
{
	args = strtok(strdup(args), " ");
	if (!checkPassword(fd, user, password)) {
		return;
	}
	if (!roomExists(args)) {
		const char * msg =  "DENIED\r\n";
		write(fd, msg, strlen(msg));
		return;
	}
	IRCVector * vect;
	if (rooms.find(args, (void **)&vect)) {
		if (!vect->find(user)) {
			const char * msg =  "ERROR (No user in room)\r\n";
			write(fd, msg, strlen(msg));
			return;
		}
	}
	rooms.find(args, (void **) &vect);
	vect->remove(user);
	users.find(user, (void **) &vect);
	vect->remove(args);
	const char * msg =  "OK\r\n";
	write(fd, msg, strlen(msg));
}

	void
IRCServer::sendMessage(int fd, const char * user, const char * password, const char * args)
{
	// message: <number> <room> <content>
	if (!checkPassword(fd, user, password)) {
		return;
	}
	if (!userIsInRoom(user, strtok(strdup(args), " "))) {
		const char * msg =  "ERROR (user not in room)\r\n";
		write(fd, msg, strlen(msg));
		return;
	}
	if (!roomExists(strtok(strdup(args), " "))) {
		const char * msg =  "DENIED\r\n";
		write(fd, msg, strlen(msg));
		return;
	}
	long messageNum;
	IRCVector * vect;
	mess.find(strtok(strdup(args), " "), (void **) &vect);
	IRCVectorIterator it(vect);
	string * content;
	bool flage = false;
	while (it.next(content)) {
		flage = true;
		if (strlen(content->c_str()) > 0)
			messageNum = strtol(strtok(strdup(content->c_str()), " "), NULL, 10);
	}
	if (flage)
		messageNum++;
	string messageStr(std::to_string(messageNum));
	int trashSize = strlen(strtok(strdup(args), " ")) + 1;
	string msgStr = args;
	if (trashSize >= msgStr.size()) {
		const char * msg =  "DENIED\r\n";
		write(fd, msg, strlen(msg));
		return;
	}
	msgStr = msgStr.substr(trashSize);
	messageStr.append(" ").append(user).append(" ").append(msgStr);
	vect->insert(&messageStr);
	const char * msg =  "OK\r\n";
	write(fd, msg, strlen(msg));
}

	void
IRCServer::getMessages(int fd, const char * user, const char * password, const char * args)
{
	//msgnum user msg
	if (!checkPassword(fd, user, password)) {
		return;
	}
	strtok(strdup(args), " ");
	if (!userIsInRoom(user, strtok(NULL, " "))) {
		const char * msg =  "ERROR (User not in room)\r\n";
		write(fd, msg, strlen(msg));
		return;
	}
	strtok(strdup(args), " ");
	if (!roomExists(strtok(NULL, " "))) {
		//swap msgNum zand room`
		const char * msg =  "DENIED\r\n";
		write(fd, msg, strlen(msg));
		return;
	}
	string out;
	IRCVector * vect;
	long lastMsg = strtol(strtok(strdup(args), " "), NULL, 10);
	mess.find(strtok(NULL, " "), (void **) &vect);
	long messageNum;
	IRCVectorIterator it(vect);
	string * content;
	while (it.next(content)) {
		if (strlen(content->c_str()) > 0) {
			messageNum = strtol(strtok(strdup(content->c_str()), " "), NULL, 10);
			if (messageNum >= lastMsg)
				out.append(*content).append("\r\n");
		}
	}
	if (out.length() == 0) {
		out = "NO-NEW-MESSAGES";
	}
	out.append("\r\n");
	const char * msg =  out.c_str();
	write(fd, msg, strlen(msg));
}

	void
IRCServer::getUsersInRoom(int fd, const char * user, const char * password, const char * args)
{
	args = strtok(strdup(args), " ");
	if (!checkPassword(fd, user, password)) {
		return;
	}
	IRCVector * vect;
	rooms.find(args, (void **) &vect);
	const char * msg =  vect->sort().append("\r\n").c_str();
	write(fd, msg, strlen(msg));
}

	void
IRCServer::listRooms(int fd, const char * user, const char * password,const  char * args)
{
	if (!checkPassword(fd, user, password)) {
		return;
	}
	const char * key;
	void * garbo;
	IRCVector rms;
	HashTableVoidIterator it(&rooms);
	while (it.next(key, garbo)) {
		if (key != NULL) {
			string pb(key);
			rms.insert(&pb);
		}
	}
	const char * msg =  rms.sort().c_str();
	write(fd, msg, strlen(msg));
}
	void
IRCServer::getAllUsers(int fd, const char * user, const char * password,const  char * args)
{
	if (!checkPassword(fd, user, password)) {
		return;
	}
	string out;
	fstream fileStream;
	fileStream.open(PASSWORD_FILE, fstream::out | fstream::in | fstream::app);
	char line[500];
	char * usr;
	IRCVector collector;
	while (fileStream.getline(line, 500)) {
		usr = strtok(line, ":");
		string pb(usr);
		collector.insert(&pb);
	}
	fileStream.close();
	const char * msg =  collector.sort().append("\r\n").c_str();
	write(fd, msg, strlen(msg));
}

