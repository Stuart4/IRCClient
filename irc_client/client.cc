#include <gtk/gtk.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <fstream>
#include "client.h"
#include "IRCVector.h"

using namespace std;
//TODO when should we leave a room?
int open_client_socket(char * host, int port) {
	// Initialize socket address structure
	struct  sockaddr_in socketAddress;
	
	// Clear sockaddr structure
	memset((char *)&socketAddress,0,sizeof(socketAddress));
	
	// Set family to Internet 
	socketAddress.sin_family = AF_INET;
	
	// Set port
	socketAddress.sin_port = htons((u_short)port);
	
	// Get host table entry for this host
	struct  hostent  *ptrh = gethostbyname(host);
	if ( ptrh == NULL ) {
		return -1;
	}
	
	// Copy the host ip address to socket address structure
	memcpy(&socketAddress.sin_addr, ptrh->h_addr, ptrh->h_length);
	
	// Get TCP transport protocol entry
	struct  protoent *ptrp = getprotobyname("tcp");
	if ( ptrp == NULL ) {
		return -1;
	}
	
	// Create a tcp socket
	int sock = socket(PF_INET, SOCK_STREAM, ptrp->p_proto);
	if (sock < 0) {
		return -1;
	}
	
	// Connect the socket to the specified server
	if (connect(sock, (struct sockaddr *)&socketAddress,
		    sizeof(socketAddress)) < 0) {
		return -1;
	}
	
	return sock;
}

#define MAX_RESPONSE (10 * 1024)
int sendCommand(char *  host, int port, char * command, char * response) {
	
	int sock = open_client_socket( host, port);

	if (sock<0) {
		return 0;
	}

	// Send command
	write(sock, command, strlen(command));
	write(sock, "\r\n",2);

	//Print copy to stdout
	write(1, command, strlen(command));
	write(1, "\r\n",2);

	// Keep reading until connection is closed or MAX_REPONSE
	int n = 0;
	int len = 0;
	while ((n=read(sock, response+len, MAX_RESPONSE - len))>0) {
		len += n;
	}
	response[len]=0;

	printf("response:\n%s\n", response);

	close(sock);

	return 1;
}

void serverSelected (GtkTreeView * tree_view, GtkTreePath * path, GtkTreeViewColumn * column, gpointer user_data) {
	GtkTreeSelection *selection;
	GtkTreeIter iterator;
	GtkTreeModel * modelo = GTK_TREE_MODEL(serverModel);
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree_view));
	if (gtk_tree_selection_get_selected(selection, &modelo, &iterator)) {
		gchar *name;
		gtk_tree_model_get (modelo, &iterator, 0, &name, -1);
		if (name == NULL) return;
		char * host = strtok(strdup(name), ":");
		char * port = strtok(NULL, "");
		gtk_entry_buffer_set_text(GTK_ENTRY_BUFFER(hostBuffer), host, -1);
		gtk_entry_buffer_set_text(GTK_ENTRY_BUFFER(portBuffer), port, -1);
		g_free(name);
	}

}

gboolean serverSelectorTick() {

	FILE *output = popen("ps -aux | grep -o 'IRCServer [0-9][0-9]*$' |grep -o '[0-9]*' | sed '    s/^/localhost:/g'", "r");
	gtk_list_store_clear(serverModel);
	GtkTreeIter iterator;
	char line[256];
	while (fgets(line, sizeof(line), output)) {
		gchar *msg = g_strdup(line);
		gchar *end = msg;
		while (*end != '\0') end++;
		end--;
		*end = '\0';
		if (strlen(msg) > 1) {
			gtk_list_store_append(GTK_LIST_STORE (serverModel), &iterator);
			gtk_list_store_set (GTK_LIST_STORE (serverModel), &iterator, 0,msg, -1);
		}
		g_free(msg);
	}
	pclose(output);
	return updatingServers;
}

static void sendMessage()
{
	if (user == NULL || password == NULL || host == NULL || port == NULL || room == NULL)
	{
		gtk_entry_buffer_set_text(sendBuffer, "", 0);
		setStatus("Not Connected...");
		return;
	}
	char response[MAX_RESPONSE];
	string command("SEND-MESSAGE ");
	command.append(*user).append(" ").append(*password).append(" ").append(*room).append(" ").append(gtk_entry_buffer_get_text(sendBuffer));
	sendCommand(strdup(host->c_str()), *port, strdup(command.c_str()), response);
	gtk_entry_buffer_set_text(sendBuffer, "", 0);
}

static void getMessages() {
	if (user == NULL || password == NULL || host == NULL || port == NULL || room == NULL) {
		if (user != NULL && password != NULL && host != NULL && port != NULL){
			string stat(*host);
			stat.append(":").append(to_string(*port));
			setStatus(stat);
			return;
		}
		setStatus("Not Connected...");
		return;
	}

	char response[MAX_RESPONSE];
	string command("GET-MESSAGES ");
	command.append(*user).append(" ").append(*password).append(" ").append(to_string(*msgNum)).append(" ").append(*room);
	sendCommand(strdup(host->c_str()), *port, strdup(command.c_str()), response);

	string userName;
	istringstream resp(response);
	GtkTextIter iterator;
	GtkTextIter tagIt;
	gtk_text_buffer_create_tag(messagesBuffer, "bold", "weight", PANGO_WEIGHT_BOLD, NULL);
	gtk_text_buffer_get_end_iter(messagesBuffer, &iterator);
	gtk_text_buffer_get_end_iter(messagesBuffer, &tagIt);
	while (getline(resp, userName)) {
		if (userName.compare("\r") == 0) continue;
		if (userName.compare("NO-NEW-MESSAGES\r") == 0) {
			*msgNum = *msgNum - 1;
			continue;
		}
		*msgNum = strtol(strtok(strdup(userName.c_str()), " "), NULL, 10);
		char * user = strtok(NULL, " ");
		char * msg = strtok(NULL, "");
		gtk_text_buffer_insert_with_tags_by_name(messagesBuffer, &iterator, user, -1, "bold", NULL);
		gtk_text_buffer_insert_with_tags_by_name(messagesBuffer, &iterator, " : ", -1, "bold", NULL);
		gtk_text_buffer_insert(messagesBuffer, &iterator, msg, -1);
	}
	*msgNum = *msgNum + 1;
}
static void roomSubmit(GtkWidget *widget, gpointer data) {
	gtk_widget_destroy(widget);
	string roomStr(gtk_entry_buffer_get_text(roomBuffer));
	if (user == NULL || password == NULL || host == NULL || port == NULL || roomStr.size() < 1) {
		setStatus("Not Connected...");
		return;
	}
	char response[MAX_RESPONSE];
	string command("CREATE-ROOM ");
	command.append(*user).append(" ").append(*password).append(" ").append(roomStr);;
	sendCommand((char *) host->c_str(), *port, strdup(command.c_str()), response);
}

static void roomPopup(GtkWidget *widget, gpointer data) {
	GtkWidget *dialog, *content_area;
	GtkWidget *label;
	GtkWidget *roomEntry;
	GtkWidget *vbox;
	GtkDialogFlags flags;
	flags = GTK_DIALOG_DESTROY_WITH_PARENT;
	dialog = gtk_dialog_new_with_buttons("Enter information", NULL, flags, "OK", GTK_RESPONSE_NONE, NULL);
	content_area = gtk_dialog_get_content_area(GTK_DIALOG (dialog));
	label = gtk_label_new ("Room: ");
	roomBuffer = gtk_entry_buffer_new(NULL, -1);
	roomEntry = gtk_entry_new_with_buffer(GTK_ENTRY_BUFFER (roomBuffer));
	vbox = gtk_vbox_new(TRUE, 1);
	gtk_box_pack_start(GTK_BOX(vbox), label, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), roomEntry, TRUE, TRUE, 0);

	g_signal_connect_swapped(dialog, "response", G_CALLBACK(roomSubmit), dialog);
	gtk_container_add(GTK_CONTAINER(content_area), vbox);
	gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
	gtk_widget_show_all(dialog);
}

static void serverSelectorClose(GtkWidget *widget, gpointer data) {
	updatingServers = false;
	gtk_widget_destroy(widget);
}

static void loginSubmit(GtkWidget *widget, gpointer data) {
	delete user;
	user = NULL;
	delete password;
	password = NULL;
	delete host;
	host = NULL;
	delete port;
	port = NULL;
	delete room;
	room = NULL;
	*msgNum = 0;
	clearRooms();
	clearUsers();
	gtk_text_buffer_set_text(messagesBuffer, "", 0);
	gtk_widget_destroy(widget);
	user = new string(gtk_entry_buffer_get_text(userBuffer));
	password = new string(gtk_entry_buffer_get_text(passBuffer));
	port = new int(strtol(gtk_entry_buffer_get_text(portBuffer), NULL, 10));
	host = new string(gtk_entry_buffer_get_text(hostBuffer));
	string title(*host);
	title.append(":").append(to_string(*port));
	gtk_window_set_title(GTK_WINDOW(window), title.c_str());
	setStatus(title);

	char response[MAX_RESPONSE];
	string command("ADD-USER ");
	command.append(*user).append(" ").append(*password);
	sendCommand((char *) host->c_str(), *port, strdup(command.c_str()), response);
	if (strcmp(response, "OK\r\n") != 0) {
		delete user;
		user = NULL;
		delete password;
		password = NULL;
		delete host;
		host = NULL;
		delete port;
		port = NULL;
	}

}
static void serverSelector( GtkWidget *widget,
		gpointer   data ) {
	updatingServers = true;
	GtkWidget *dialog, *content_area;
	g_timeout_add(1000, (GSourceFunc) serverSelectorTick, (gpointer) dialog);
	GtkWidget *upperLabel, *midTreeView, *lowerCloseButton;
	GtkWidget *midScrollBars;
	GtkWidget *vbox;
	GtkDialogFlags flags;
	flags = GTK_DIALOG_DESTROY_WITH_PARENT;
	dialog = gtk_dialog_new_with_buttons("Local Servers", NULL, flags, "Close", GTK_RESPONSE_REJECT, NULL);
	content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
	upperLabel = gtk_label_new ("Select Server:");
	midScrollBars = gtk_scrolled_window_new(NULL, NULL);
	vbox = gtk_vbox_new(TRUE, 1);
	serverModel = gtk_list_store_new(1, G_TYPE_STRING);
	serverRenderer = gtk_cell_renderer_text_new();
	serverColumn = gtk_tree_view_column_new_with_attributes("Servers", serverRenderer, "text", 0, NULL);
	midTreeView = gtk_tree_view_new_with_model(GTK_TREE_MODEL(serverModel));
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(midScrollBars), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_tree_view_append_column(GTK_TREE_VIEW(midTreeView), GTK_TREE_VIEW_COLUMN(serverColumn));
	gtk_container_add(GTK_CONTAINER (midScrollBars), midTreeView);
	gtk_box_pack_start(GTK_BOX(vbox), upperLabel, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), midScrollBars, TRUE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(content_area), vbox);
	gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
	g_signal_connect_swapped(dialog, "response", G_CALLBACK(serverSelectorClose), dialog);
	g_signal_connect(midTreeView, "row-activated", G_CALLBACK(serverSelected), NULL);
	gtk_widget_show_all(dialog);
}
static void login( GtkWidget *widget,
		gpointer   data )
{
	GtkWidget *dialog, *content_area, *action_area;
	GtkWidget *upperLabel, *lowerLabel, *portLabel, *hostLabel;
	GtkWidget *upperTextView, *lowerTextView, *hostTextView, *portTextView;
	GtkWidget *vbox;
	GtkWidget *serverSelectorButton;
	GtkDialogFlags flags;
	flags = GTK_DIALOG_DESTROY_WITH_PARENT;
	dialog = gtk_dialog_new_with_buttons("Enter information", NULL, flags, "OK", GTK_RESPONSE_NONE, NULL);
	serverSelectorButton = gtk_button_new_with_label("Server Selector");
	content_area = gtk_dialog_get_content_area(GTK_DIALOG (dialog));
	action_area = gtk_dialog_get_action_area(GTK_DIALOG (dialog));
	upperLabel = gtk_label_new ("Username: ");
	lowerLabel = gtk_label_new ("Password: ");
	hostLabel = gtk_label_new ("Host: ");
	portLabel = gtk_label_new ("Port: ");
	userBuffer = gtk_entry_buffer_new(NULL, -1);
	passBuffer = gtk_entry_buffer_new(NULL, -1);
	hostBuffer = gtk_entry_buffer_new(NULL, -1);
	portBuffer = gtk_entry_buffer_new(NULL, -1);
	upperTextView = gtk_entry_new_with_buffer(GTK_ENTRY_BUFFER (userBuffer));
	lowerTextView = gtk_entry_new_with_buffer(GTK_ENTRY_BUFFER (passBuffer));
	hostTextView = gtk_entry_new_with_buffer(GTK_ENTRY_BUFFER (hostBuffer));
	portTextView = gtk_entry_new_with_buffer(GTK_ENTRY_BUFFER (portBuffer));
	vbox = gtk_vbox_new(TRUE, 1);
	gtk_box_pack_start(GTK_BOX(vbox), upperLabel, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), upperTextView, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), lowerLabel, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), lowerTextView, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hostLabel, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hostTextView, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), portLabel, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), portTextView, TRUE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(action_area), serverSelectorButton);

	g_signal_connect_swapped(dialog, "response", G_CALLBACK(loginSubmit), dialog);
	g_signal_connect(serverSelectorButton, "clicked", G_CALLBACK(serverSelector), NULL);
	gtk_container_add(GTK_CONTAINER(content_area), vbox);
	gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
	gtk_widget_show_all(dialog);
}

void clearUsers() {
	gtk_list_store_clear(userModel);
}

void clearRooms() {
	gtk_list_store_clear(roomModel);
}

void addToUsers(std::string user) {
	if (user.compare("\r") == 0 || user.compare("DENIED\r\n") == 0) return;
	gtk_text_buffer_create_tag(messagesBuffer, "bold", "weight", PANGO_WEIGHT_BOLD, NULL);
	GtkTreeIter iterator;
	gchar *msg = g_strdup(user.c_str());
	gtk_list_store_append(GTK_LIST_STORE (userModel), &iterator);
	gtk_list_store_set (GTK_LIST_STORE (userModel), &iterator, 0,msg, -1);
	g_free(msg);
}

void addToRooms(std::string room) {
	if (room.compare("\r") == 0 || room.compare("DENIED\r\n") == 0) return;
	gtk_text_buffer_create_tag(messagesBuffer, "bold", "weight", PANGO_WEIGHT_BOLD, NULL);
	GtkTreeIter iterator;
	gchar *msg = g_strdup(room.c_str());
	gtk_list_store_append(GTK_LIST_STORE (roomModel), &iterator);
	gtk_list_store_set (GTK_LIST_STORE (roomModel), &iterator, 0,msg, -1);
	g_free(msg);
}

void leaveCurrRoom() {
	if (host == NULL || port == NULL || user == NULL || password == NULL || room == NULL) return;
	char response[MAX_RESPONSE];
	string command("LEAVE-ROOM ");
	command.append(*user).append(" ").append(*password).append(" ").append(*room);
	sendCommand((char *) host->c_str(), *port, strdup(command.c_str()), response);
}

void roomSelected (GtkTreeView * tree_view, GtkTreePath * path, GtkTreeViewColumn * column, gpointer user_data) {
	*msgNum = 0;
	delete userVect;
	leaveCurrRoom();
	userVect = new IRCVector();
	gtk_text_buffer_set_text(messagesBuffer, "", 0);
	GtkTreeSelection *selection;
	GtkTreeIter iterator;
	GtkTreeModel * modelo = GTK_TREE_MODEL(roomModel);
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree_view));
	if (gtk_tree_selection_get_selected(selection, &modelo, &iterator)) {
		gchar *name;
		gtk_tree_model_get (modelo, &iterator, 0, &name, -1);
		room = new string(name);
		g_free(name);
	}
	string title(*room);
	title.append(" - ").append(*host).append(":").append(to_string(*port));
	gtk_window_set_title(GTK_WINDOW (window), title.c_str());
	setStatus(title);
	char response[MAX_RESPONSE];
	string command("ENTER-ROOM ");
	if (room == NULL || user == NULL || password == NULL || host == NULL || port == NULL) {
		setStatus("Not Connected...");
		return;
	}
	command.append(*user).append(" ").append(*password).append(" ").append(*room);
	sendCommand((char *) host->c_str(), *port, strdup(command.c_str()), response);

}

void setStatus(string status) {
	gtk_label_set_text(GTK_LABEL(statusLabel), status.c_str());
}

bool isUserNew(string user) {
	GtkTreeIter iterator;
	GtkTreeModel * model = GTK_TREE_MODEL(userModel);
	if (gtk_tree_model_get_iter_first(model, &iterator)) {
		
	}
}

void printUserLeft(string user) {
	GtkTextIter iterator;
	gtk_text_buffer_get_end_iter(messagesBuffer, &iterator);
	gtk_text_buffer_insert(messagesBuffer, &iterator, "< ", -1);
	gtk_text_buffer_insert_with_tags_by_name(messagesBuffer, &iterator, user.c_str(), -1, "bold", NULL);
	gtk_text_buffer_insert(messagesBuffer, &iterator, " has left the room >\n", -1);
}

void printUserEntered(string user) {
	GtkTextIter iterator;
	gtk_text_buffer_get_end_iter(messagesBuffer, &iterator);
	gtk_text_buffer_insert(messagesBuffer, &iterator, "< ", -1);
	gtk_text_buffer_insert_with_tags_by_name(messagesBuffer, &iterator, user.c_str(), -1, "bold", NULL);
	gtk_text_buffer_insert(messagesBuffer, &iterator, " has entered the room >\n", -1);
}

void updateUsersAndRooms() {
	clearRooms();
	clearUsers();
	if (user == NULL || password == NULL || host == NULL || port == NULL) {
		setStatus("Not Connected...");
		return;
	}

	if (room != NULL){
		char response[MAX_RESPONSE];
		string command("GET-USERS-IN-ROOM ");
		command.append(*user).append(" ").append(*password).append(" ").append(*room);
		sendCommand((char *) host->c_str(), *port, strdup(command.c_str()), response);
		string userName;
		istringstream resp(response);
		IRCVector * newVect = new IRCVector();
		bool flag = true;
		if (userVect->getSize() == 0) flag = false;
		while (getline(resp, userName)){
			if (userName.compare("\r") == 0) continue;
			if (!userVect->find(userName.substr(0, userName.size() - 1))) {
					//print user entered
					if (flag)
						printUserEntered(userName.substr(0, userName.size() - 1));
			} else {
				userVect->remove(userName.substr(0, userName.size() - 1));
					}
			string * tmp = new string(userName.substr(0, userName.size() - 1));
			newVect->insert(tmp);
			tmp = new string(userName.substr(0, userName.size() - 1));
			addToUsers(*tmp);
		}
		//print userVect.sort()
		IRCVectorIterator iterator(userVect);
		string * data;
		while (iterator.next(data)) printUserLeft(*data);
		delete userVect;
		userVect = newVect;
	}
	{
		char response[MAX_RESPONSE];
		string command("LIST-ROOMS ");
		command.append(*user).append(" ").append(*password);
		sendCommand((char *) host->c_str(), *port, strdup(command.c_str()), response);
		string userName;
		istringstream resp(response);
		while (getline(resp, userName)) addToRooms(userName.substr(0, userName.size() - 1));
	}
}

gboolean onTime() {
	updateUsersAndRooms();
	getMessages();
	return true;
}

static gboolean delete_event( GtkWidget *widget,
		GdkEvent  *event,
		gpointer   data )
{
	/* If you return FALSE in the "delete-event" signal handler,
	 * GTK will emit the "destroy" signal. Returning TRUE means
	 * you don't want the window to be destroyed.
	 * This is useful for popping up 'are you sure you want to quit?'
	 * type dialogs. */

	destroy(NULL, NULL);

	/* Change TRUE to FALSE and the main window will be destroyed with
	 * a "delete-event". */

	return TRUE;
}

/* Another callback */
static void destroy( GtkWidget *widget,
		gpointer   data )
{
	leaveCurrRoom();
	gtk_main_quit ();
}
int gtkMain( int   argc,
		char *argv[] )
{
	/* GtkWidget is the storage type for widgets */
	GtkWidget *sendButton;
	GtkWidget *loginButton;
	GtkWidget *createLoginButton;
	GtkWidget *vbox;
	GtkWidget *lowerHbox;
	GtkWidget *upperHbox;
	GtkWidget *midVbox;
	GtkWidget *lowerVbox;
	GtkWidget *lowerText;
	GtkWidget *midText;
	GtkWidget *leftList;
	GtkWidget *rightList;
	GtkWidget *midScrollBars;
	GtkWidget *rightScrollBars;
	GtkWidget *leftScrollBars;
	GtkWidget *leftFrame;
	GtkWidget *rightFrame;
	GtkWidget *midFrame;
	GtkWidget *lowerFrame;
	GtkWidget *leftTree;
	GtkWidget *rightTree;
	GtkWidget *createRoomButton;


	/* This is called in all GTK applications. Arguments are parsed
	 * from the command line and are returned to the application. */
	gtk_init (&argc, &argv);

	/* create a new window */
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	vbox = gtk_vbox_new (FALSE, 1);
	lowerHbox = gtk_hbox_new (TRUE, 1);
	upperHbox = gtk_hbox_new (TRUE, 1);
	midVbox = gtk_vbox_new (TRUE, 1);
	lowerVbox = gtk_vbox_new (TRUE, 1);
	sendBuffer = gtk_entry_buffer_new("", 0);
	messagesBuffer = gtk_text_buffer_new(NULL);
	lowerText = gtk_entry_new_with_buffer(sendBuffer);
	statusLabel = gtk_label_new("Not Connected...");
	midText = gtk_text_view_new_with_buffer(messagesBuffer);
	gtk_text_view_set_editable(GTK_TEXT_VIEW (midText), FALSE);
	midScrollBars = gtk_scrolled_window_new(NULL, NULL);
	leftScrollBars = gtk_scrolled_window_new(NULL, NULL);
	rightScrollBars = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(leftScrollBars), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(rightScrollBars), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	midFrame = gtk_frame_new(NULL);
	lowerFrame = gtk_frame_new(NULL);
	gtk_frame_set_label(GTK_FRAME (midFrame), "Messages:");
	gtk_frame_set_label(GTK_FRAME (lowerFrame), "Type a message:");
	userModel = gtk_list_store_new (1, G_TYPE_STRING);
	roomModel = gtk_list_store_new (1, G_TYPE_STRING);
	leftTree = gtk_tree_view_new_with_model(GTK_TREE_MODEL (roomModel));
	rightTree = gtk_tree_view_new_with_model(GTK_TREE_MODEL (userModel));
	rightRenderer = gtk_cell_renderer_text_new();
	leftRenderer = gtk_cell_renderer_text_new();
	leftColumn = gtk_tree_view_column_new_with_attributes("Rooms", leftRenderer, "text", 0, NULL);
	rightColumn = gtk_tree_view_column_new_with_attributes("Users", rightRenderer, "text", 0, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (rightTree), GTK_TREE_VIEW_COLUMN (rightColumn));
	gtk_tree_view_append_column (GTK_TREE_VIEW (leftTree), GTK_TREE_VIEW_COLUMN (leftColumn));
	gtk_label_set_justify(GTK_LABEL(statusLabel), GTK_JUSTIFY_LEFT);

	/* When the window is given the "delete-event" signal (this is given
	 * by the window manager, usually by the "close" option, or on the
	 * titlebar), we ask it to call the delete_event () function
	 * as defined above. The data passed to the callback
	 * function is NULL and is ignored in the callback function. */
	g_signal_connect (window, "delete-event",
			G_CALLBACK (delete_event), NULL);

	/* Here we connect the "destroy" event to a signal handler.  
	 * This event occurs when we call gtk_widget_destroy() on the window,
	 * or if we return FALSE in the "delete-event" callback. */
	g_signal_connect (window, "destroy",
			G_CALLBACK (destroy), NULL);

	/* Sets the border width of the window. */
	gtk_container_set_border_width (GTK_CONTAINER (window), 10);

	/* Creates a new button with the label "Hello World". */
	sendButton = gtk_button_new_with_label ("Send Message");
	createRoomButton = gtk_button_new_with_label("Create A Room");
	loginButton = gtk_button_new_with_label ("Login");
	createLoginButton = gtk_button_new_with_label("Sign Up");

	/* When the button receives the "clicked" signal, it will call the
	 * function hello() passing it NULL as its argument.  The hello()
	 * function is defined above. */
	g_signal_connect (sendButton, "clicked",
			G_CALLBACK (sendMessage), NULL);
	g_signal_connect (loginButton, "clicked",
			G_CALLBACK (login), NULL);
	g_signal_connect (createLoginButton, "clicked",
			G_CALLBACK (login), NULL);
	g_signal_connect (createRoomButton, "clicked",
			G_CALLBACK (roomPopup), NULL);
	g_signal_connect(leftTree, "row-activated", G_CALLBACK(roomSelected), NULL);

	/* This will cause the window to be destroyed by calling
	 * gtk_widget_destroy(window) when "clicked".  Again, the destroy
	 * signal could come from here, or the window manager. */

	/* This packs the button into the window (a gtk container). */
	gtk_container_add (GTK_CONTAINER (window), vbox);
	gtk_box_pack_start (GTK_BOX (vbox), upperHbox, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), midVbox, TRUE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER (midScrollBars), midText);
	gtk_container_add(GTK_CONTAINER (midFrame), midScrollBars);
	gtk_box_pack_start (GTK_BOX (midVbox), midFrame, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), lowerVbox, TRUE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER (lowerFrame), lowerText);
	gtk_box_pack_start (GTK_BOX (lowerVbox), lowerFrame, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (lowerVbox), lowerText, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (lowerVbox), lowerText, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), lowerHbox, TRUE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER (leftScrollBars), leftTree);
	gtk_box_pack_start (GTK_BOX (upperHbox), leftScrollBars, TRUE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER (rightScrollBars), rightTree);
	gtk_box_pack_start (GTK_BOX (upperHbox), rightScrollBars, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (lowerHbox), sendButton, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (lowerHbox), createRoomButton, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (lowerHbox), loginButton, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (lowerHbox), createLoginButton, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), statusLabel, TRUE, TRUE, 0);

	/* The final step is to display this newly created widget. */

	//timer
	g_timeout_add(1000, (GSourceFunc) onTime, (gpointer) window);

	/*gtk_widget_show (vbox);
	gtk_widget_show (sendButton);
	gtk_widget_show (loginButton);
	gtk_widget_show (createAccountButton); */

	/* and the window */
	gtk_widget_show_all (window);

	/* All GTK applications must have a gtk_main(). Control ends here
	 * and waits for an event to occur (like a key press or
	 * mouse event). */
	gtk_main ();

	return 0;
}
int
main(int argc, char **argv) {

	user = NULL;
	password = NULL;
	host = NULL;
	port = NULL;
	msgNum = new int();
	room = NULL;
	userVect = new IRCVector();
	updatingServers = false;


	//char response[MAX_RESPONSE];
	//sendCommand(host, port, command, response);
	gtkMain(argc, argv);

	return 0;
}



