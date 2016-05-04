default: libsocket++.a

libsocket++.a: ServerSocket.o Socket.o ClientSocket.o
	ar cru libsocket++.a Socket.o ServerSocket.o ClientSocket.o

SocketServer.o: ServerSocket.cpp
	g++ -g -c ServerSocket.cpp

SocketClient.o: ClientSocket.cpp
	g++ -g -c ClientSocket.cpp

Socket.o: Socket.cpp
	g++ -g -c Socket.cpp
