CC :=gcc
CPP :=g++
LDFLAGS :=
CFLAGS :=-pthread
C_SOURCES :=$(wildcard *.c)
C_EXECUTABLE :=$(C_SOURCES:.c=)
CPP_SOURCES :=$(wildcard *.cpp)
CPP_EXECUTABLE :=$(CPP_SOURCES:.cpp=)
 
all:$(C_EXECUTABLE) $(CPP_EXECUTABLE)

$(C_EXECUTABLE):%:%.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

$(CPP_EXECUTABLE):%:%.cpp
	$(CPP) $(CFLAGS) $< -o $@ $(LDFLAGS)

clean:
	rm -rf $(C_EXECUTABLE) $(CPP_EXECUTABLE)