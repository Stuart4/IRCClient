#ifndef Client_H
#define Client_H

#include <string>
#include <gtk/gtk.h>
#include "IRCVector.h"
class IRCVector;
std::string * user;
std::string * password;
std::string * host;
std::string * room;
int * port;
int * msgNum;
IRCVector * userVect;
GtkEntryBuffer *userBuffer, *passBuffer, *hostBuffer, *portBuffer, *sendBuffer, *roomBuffer;
GtkTextBuffer *messagesBuffer;
GtkListStore *userModel;
GtkListStore *roomModel;
GtkListStore *serverModel;
GtkCellRenderer *leftRenderer;
GtkTreeViewColumn *leftColumn;
GtkCellRenderer *rightRenderer;
GtkTreeViewColumn *rightColumn;
GtkCellRenderer *serverRenderer;
GtkTreeViewColumn *serverColumn;
GtkWidget *window;
GtkWidget *statusLabel;
bool updatingServers;

int open_client_socket(char * host, int port);
int sendCommand(char *  host, int port, char * command, char * response);
void addToUsers(std::string user);
static void serverSelectorClose(GtkWidget *widget, gpointer data);
void clearUsers();
gboolean serverSelectorTick();
void printUserLeft(std::string);
void printUserEntered(std::string);
bool isUserNew(std::string userName);
void leaveCurrRoom();
void setStatus(std::string status);
void addToRooms(std::string room);
static void getMessages();
gboolean onTime();
void clearRooms();
void updateUsersAndRooms();
void roomSelected (GtkTreeView * tree_view, GtkTreePath * path, GtkTreeViewColumn * column, gpointer user_data);
void serverSelected (GtkTreeView * tree_view, GtkTreePath * path, GtkTreeViewColumn * column, gpointer user_data);
static void sendMessage();
static gboolean delete_event( GtkWidget *widget, GdkEvent *event,gpointer data);
static void loginSubmit( GtkWidget *widget, gpointer data);
static void serverSelector(GtkWidget *widget, gpointer data);
static void login(GtkWidget *widget, gpointer data);
static void roomSubmit( GtkWidget *widget, gpointer data);
static void roomPopup(GtkWidget *widget, gpointer data);
static void destroy( GtkWidget *widget, gpointer data);
int gtkMain(int argc, char *argv[]);
int main(int argc, char **argv);

#endif
