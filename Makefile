all:external_sort server client
external_sort:external_sort.cpp
	g++ -o external_sort external_sort.cpp
server:server.cpp
	g++ -o server server.cpp
client:client.cpp
	g++ -o client client.cpp
clean:
	rm -f external_sort server client