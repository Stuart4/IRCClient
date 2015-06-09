
CXX = g++ -fPIC

all: IRCServer

IRCServer: IRCServer.cc IRCVector.cc
	g++ -std=c++11 -g -o IRCServer IRCServer.cc IRCVector.cc

clean:
	rm -f {p,}assword.txt
	rm -f IRCServer


