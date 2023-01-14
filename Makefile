all:external_sort external_sort_mt master slave main
external_sort:external_sort.cpp
	g++ -o external_sort external_sort.cpp
external_sort_mt:external_sort_mt.cpp
	g++ -o external_sort_mt external_sort_mt.cpp
master:master.cpp
	g++ -o master master.cpp
slave:slave.cpp
	g++ -o slave slave.cpp
main:main.cpp
	g++ -o main main.cpp
clean:
	rm -f external_sort external_sort_mt master slave main