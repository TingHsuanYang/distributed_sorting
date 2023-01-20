objects = master.o slave.o external_sort.o external_sort_mt.o

all:clean main
main:main.cpp $(objects)
	g++ -o main main.cpp $(objects) -pthread
master:master.cpp master.hpp
	g++ -o master  master.cpp -pthread
slave:slave.cpp slave.hpp external_sort.hpp external_sort.o external_sort_mt.o
	g++ -o slave slave.cpp external_sort.o external_sort_mt.o -pthread

external_sort.o:external_sort.hpp
external_sort_mt.o:external_sort_mt.hpp
master.o:master.hpp
slave.o:slave.hpp

.PHONY:clean
clean:
	-rm main $(objects)
