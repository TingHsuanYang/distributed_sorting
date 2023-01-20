#include <cstring>
#include <string>
#include <vector>

#define DATA_SIZE 100

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

class ExternalSortMT {
   public:
    ExternalSortMT(std::string inputName, std::string outputName);
    ~ExternalSortMT();
    int run();

   private:
    std::string inputName;
    std::string outputName;
    std::vector<std::string> part_names;
    void thread_process(long long curPos, long long size, int thread_id);
    void thread_merge(std::vector<std::string>& thread_part_names, int thread_id);
    void merge();
};