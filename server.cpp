/**
 * Server splits the large file into small parts and transfers then to slaves.
 * Later will receive the sorted parts from slaves and merge them into one file.
 * The sorting processes happen concurrently.
 * Here we can see the overhead of transferring files.
 */
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>
#include <string>

using namespace std;

int main(int argc, char const* argv[]) {
    // AF_INET = IPv4, SOCK_STREAM = TCP
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (socket_fd < 0) {
        printf("Fail to create a socket.");
    }

    // close socket
    close(socket_fd);

    /* code */
    return 0;
}
