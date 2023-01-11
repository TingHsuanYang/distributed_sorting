all:external_sort external_sort_mt server slave
external_sort:external_sort.cpp
	g++ -o external_sort external_sort.cpp
external_sort_mt:external_sort_mt.cpp
	g++ -o external_sort_mt external_sort_mt.cpp
server:server.cpp
	g++ -o server server.cpp
slave:slave.cpp
	g++ -o slave slave.cpp
clean:
	rm -f external_sort external_sort_mt server slave