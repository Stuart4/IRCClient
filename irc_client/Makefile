
goal: client

client: client.cc client.h IRCVector.cc IRCVector.h
	g++ -g -std=c++11 IRCVector.cc client.cc -o client `pkg-config --cflags --libs gtk+-2.0`

clean:
	rm -f client

