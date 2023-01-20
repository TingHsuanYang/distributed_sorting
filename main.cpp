/**
 * entry point of the program
 * ./main -m master -p 8080 -n 5 -i ./input -o ./output
 * ./main --mode master --port 8080 --num 5 --input ./input --output ./output
 * ./main -m slave -s 127.0.0.1 -p 8080
 * ./main --mode slave --server 127.0.0.1 --port 8080
 *
 */

#include <getopt.h>
#include <unistd.h>

#include <iostream>
#include <string>
#include <vector>

#include "external_sort.hpp"
#include "external_sort_mt.hpp"
#include "master.hpp"
#include "slave.hpp"

using namespace std;

void help() {
    cout << "Usage: main [-m|--mode <master|slave>] [-p|--port <port>] [-n|--num <num>] [-i|--input <input>] [-o|--output <output>]" << endl;
    cout << "Example: ./main -m master -p 8080 -n 5 -i ./input -o ./output" << endl;
    cout << "Example: ./main -m slave -p 8080" << endl;
    cout << "Example: ./main -m sort -i ./input -o ./output" << endl;
    cout << "Example: ./main -m sort_mt -i ./input -o ./output" << endl;
}

int main(int argc, char** argv) {
    static struct option long_options[] = {
        {"mode", required_argument, 0, 'm'},
        {"port", required_argument, 0, 'p'},
        {"num", required_argument, 0, 'n'},
        {"input", required_argument, 0, 'i'},
        {"output", required_argument, 0, 'o'},
        {"server", required_argument, 0, 's'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}};
    int option_index = 0;
    int c, port, num;
    string mode, input, output, server_ip;

    while ((c = getopt_long(argc, argv, "m:p:n:i:o:s:", long_options, &option_index)) != -1) {
        switch (c) {
            case 'm':
                mode = optarg;
                break;
            case 'p':
                port = atoi(optarg);
                break;
            case 'n':
                num = atoi(optarg);
                break;
            case 'i':
                input = optarg;
                break;
            case 'o':
                output = optarg;
                break;
            case 's':
                server_ip = optarg;
                break;
            case 'h':
                help();
                return 0;
            default:
                help();
                return 1;
        }
    }

    if (mode == "master") {
        if (num == 0 || input.empty() || output.empty()) {
            help();
            return 1;
        }
        Master* master = new Master(port, num, input, output);
        master->run();
        delete master;
    } else if (mode == "slave") {
        if (port == 0) {
            help();
            return 1;
        }
        Slave* slave = new Slave(server_ip, port);
        slave->run();
    } else if (mode == "sort") {
        ExternalSort* external_sort = new ExternalSort(input, output);
        external_sort->run();
        delete external_sort;
    } else if (mode == "sort_mt") {
        ExternalSortMT* external_sort_mt = new ExternalSortMT(input, output);
        external_sort_mt->run();
        delete external_sort_mt;
    } else {
        help();
        return 1;
    }

    return 0;
}
