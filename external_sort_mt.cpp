#include "external_sort_mt.hpp"

#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <cstring>
#include <fstream>
#include <iostream>
#include <queue>
#include <string>
#include <thread>
#include <vector>

#define MEMORY_SIZE 100000000  // 100 MB

using namespace std;

ExternalSortMT::ExternalSortMT(string inputName, string outputName) : inputName(inputName), outputName(outputName) {}
ExternalSortMT::~ExternalSortMT() {}

void ExternalSortMT::thread_process(long long cur_pos, long long size, int thread_id) {
    ifstream input;
    input.open(inputName, ios::in | ios::binary);
    if (!input.good()) {
        printf("Fail to open input file.\n");
        return;
    }

    // collect the part names for the thread
    vector<string> thread_part_names;

    // read the data from the input file
    char* buffer = new char[MEMORY_SIZE];
    input.seekg(cur_pos, ios::beg);
    int part_num = 0;
    while (size > 0) {
        if (size < MEMORY_SIZE)
            input.read(buffer, size);
        else
            input.read(buffer, MEMORY_SIZE);
        int read_size = input.gcount();

        // convert the buffer to record vector
        vector<Record> buffer_vector;
        for (int i = 0; i < read_size / DATA_SIZE; i++) {
            Record r;
            memcpy(r.value, buffer + i * DATA_SIZE, DATA_SIZE);
            buffer_vector.push_back(r);
        }

        // sort the buffer
        sort(buffer_vector.begin(), buffer_vector.end(), [](const Record& r1, const Record& r2) {
            return memcmp(r1.value, r2.value, DATA_SIZE) < 0;
        });

        // write the sorted buffer to the output file
        // folder for thread output
        string thread_output_folder = string("thread") + to_string(thread_id);
        // create the folder if not exist
        if (access(thread_output_folder.c_str(), F_OK) == -1) {
            mkdir(thread_output_folder.c_str(), 0777);
        }

        ofstream output;
        string part_name = thread_output_folder + "/" + string("part") + "_" + to_string(part_num);
        thread_part_names.push_back(part_name);
        output.open(part_name, ios::out | ios::binary);
        if (!output.good()) {
            printf("Fail to open output file.\n");
            return;
        }
        for (int i = 0; i < buffer_vector.size(); i++) {
            output.write(buffer_vector[i].value, DATA_SIZE);
        }
        output.close();
        size -= read_size;
        part_num++;
    }
    delete[] buffer;
    input.close();

    // merge the thread result
    thread_merge(thread_part_names, thread_id);
}

// k-way merge
void ExternalSortMT::thread_merge(vector<string>& thread_part_names, int thread_id) {
    // output the thread result
    ofstream output(string("part") + "_" + to_string(thread_id), ios::out | ios::binary);
    priority_queue<HeapNode*, vector<HeapNode*>, Comparator> heap;

    // open all the part files
    vector<ifstream> part_files;
    for (int i = 0; i < thread_part_names.size(); i++) {
        ifstream part_file(thread_part_names[i], ios::in | ios::binary);
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

    // remove the part files and the folder
    for (int i = 0; i < thread_part_names.size(); i++) {
        remove(thread_part_names[i].c_str());
    }
    remove((string("thread") + to_string(thread_id)).c_str());
}

void ExternalSortMT::merge() {
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

int ExternalSortMT::run() {
    auto start = chrono::high_resolution_clock::now();

    // number of the process cores
    int num_cores = thread::hardware_concurrency();
    int num_threads = num_cores + 2;
    printf("number of cores: %d\n", num_cores);

    // print file size in GB
    struct stat stat_buf;
    int rc = stat(inputName.c_str(), &stat_buf);
    long long file_size = rc == 0 ? stat_buf.st_size : -1;
    printf("file size: %.2f GB\n", file_size / 1024.0 / 1024.0 / 1024.0);

    // calculate the number of records per thread
    long long num_records = file_size / DATA_SIZE;
    long long num_records_per_thread = num_records / num_threads;
    int num_remaining_records = num_records % num_threads;

    vector<thread> threads;
    long long cur_pos = 0;
    for (int i = 0; i < num_threads; i++) {
        long long size = num_records_per_thread * DATA_SIZE;
        if (num_remaining_records > 0) {
            size += DATA_SIZE;
            num_remaining_records--;
        }
        threads.push_back(thread(&ExternalSortMT::thread_process, this, cur_pos, size, i));
        cur_pos += size;
    }

    // wait for all the threads to finish
    for (int i = 0; i < threads.size(); i++) {
        threads[i].join();
    }

    // get the part file names
    for (int i = 0; i < num_threads; i++) {
        part_names.push_back(string("part") + "_" + to_string(i));
    }

    // remove the input file
    remove(inputName.c_str());

    // merge the thread result
    merge();

    // remove the part files
    for (int i = 0; i < part_names.size(); i++) {
        remove(part_names[i].c_str());
    }

    auto end = chrono::high_resolution_clock::now();
    printf("execution time: %.3f seconds\n", chrono::duration_cast<chrono::milliseconds>(end - start).count() / 1000.0);

    return 0;
}
