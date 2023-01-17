#include "external_sort.hpp"

#include <sys/stat.h>

#include <algorithm>
#include <chrono>
#include <cstring>
#include <fstream>
#include <iostream>
#include <queue>
#include <string>
#include <vector>

#define DATA_SIZE 100
#define MEMORY_SIZE 100000000  // 100 MB
#define error_message "An error has occurred\n"

using namespace std;

ExternalSort::ExternalSort(string inputName, string outputName) : inputName(inputName), outputName(outputName) {}
ExternalSort::~ExternalSort() {}

struct Record {
    char value[DATA_SIZE];
};

struct HeapNode {
    int index;
    char value[DATA_SIZE];
    HeapNode(int index, char* value) {
        this->index = index;
        memcpy(this->value, value, DATA_SIZE);
    }
};

// comparator for the heap
struct Comparator {
    bool operator()(const HeapNode* node1, const HeapNode* node2) {
        return memcmp(node1->value, node2->value, DATA_SIZE) > 0;
    }
};

int ExternalSort::input() {
    ifstream input;
    input.open(inputName, ios::in | ios::binary);

    if (!input.good()) {
        printf("Fail to open input file.\n");
        return 1;
    }

    // print file size in GB
    struct stat stat_buf;
    int rc = stat(inputName.c_str(), &stat_buf);
    double file_size = rc == 0 ? stat_buf.st_size : -1;
    printf("file size: %.2f GB\n", file_size / 1024.0 / 1024.0 / 1024.0);

    ofstream output;
    int part_num = 0;
    char* buffer = new char[MEMORY_SIZE];
    while (!input.eof()) {
        input.read(buffer, MEMORY_SIZE);

        // convert the buffer to record vector
        vector<Record> buffer_vector;
        for (int i = 0; i < input.gcount() / DATA_SIZE; i++) {
            Record r;
            memcpy(r.value, buffer + i * DATA_SIZE, DATA_SIZE);
            buffer_vector.push_back(r);
        }

        sort(buffer_vector.begin(), buffer_vector.end(), [](const Record& r1, const Record& r2) {
            return memcmp(r1.value, r2.value, DATA_SIZE) <= 0;
        });

        string part_name = "part_" + to_string(part_num);
        part_names.push_back(part_name);

        // write the buffer vector to output file
        ofstream output(part_name, ios::out | ios::binary);
        for (int i = 0; i < buffer_vector.size(); i++) {
            output.write(buffer_vector[i].value, DATA_SIZE);
        }

        part_num++;
    }
    delete[] buffer;
    input.close();

    return 0;
}

// k-way merge
void ExternalSort::merge() {
    ofstream output(outputName, ios::out | ios::binary);
    priority_queue<HeapNode*, vector<HeapNode*>, Comparator> heap;

    // open all the part files
    vector<ifstream> part_files;
    for (int i = 0; i < part_names.size(); i++) {
        ifstream part_file(part_names[i], ios::in | ios::binary);
        part_files.push_back(move(part_file));
    }

    // read the first record of each part file
    char buffer[DATA_SIZE];
    for (int i = 0; i < part_files.size(); i++) {
        part_files[i].read(buffer, DATA_SIZE);
        if (part_files[i].gcount() > 0) {
            HeapNode* node = new HeapNode(i, buffer);
            heap.push(node);
        }
    }

    // get the smallest record from the heap and write it to the output file
    while (!heap.empty()) {
        HeapNode* node = heap.top();
        heap.pop();
        output.write(node->value, DATA_SIZE);

        // read the next record of the part file
        // if the part file is not empty, push the record to the heap
        part_files[node->index].read(buffer, DATA_SIZE);
        if (part_files[node->index].gcount() > 0) {
            HeapNode* next_node = new HeapNode(node->index, buffer);
            heap.push(next_node);
        }

        delete node;
    }

    // close all the part files
    for (int i = 0; i < part_files.size(); i++) {
        part_files[i].close();
    }
    output.close();
}

int ExternalSort::run() {
    auto start = chrono::high_resolution_clock::now();

    // read the file and split it into parts (sorted)
    input();

    // merge the parts and output the result
    merge();

    // remove the part files
    for (int i = 0; i < part_names.size(); i++) {
        remove(part_names[i].c_str());
    }

    auto end = chrono::high_resolution_clock::now();
    printf("sort time: %.2f s\n", chrono::duration_cast<chrono::milliseconds>(end - start).count() / 1000.0);

    return 0;
}
