#include <string>
#include <vector>

class ExternalSort {
   public:
    ExternalSort(std::string inputName, std::string outputName);
    ~ExternalSort();
    int run();

   private:
    std::string inputName;
    std::string outputName;
    std::vector<std::string> part_names;
    int input();
    void merge();
};