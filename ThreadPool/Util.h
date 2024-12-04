#include <mutex>
#include <queue>
#include <string>
#include <vector>

constexpr char DELIMITER = '\n'; // 规定中间文件的分隔符

std::string fileGen(std::string old_file, std::string new_file_name);

std::vector<std::string> splitFile(std::string filename, size_t file_size);

std::string sortFile(std::string filename);

void mergeFile(std::string first, std::string second,
               std::queue<std::string> &que, std::mutex &que_mutex,
               std::queue<std::vector<std::string>> &log_que,
               size_t cache_size);

void readFile(std::vector<char> &read_cache, std::vector<int64_t> &cache,
              std::ifstream &inFile);

// 将二进制文件转换为文本文件，查看结果
std::string bin2Text(std::string origin_file, size_t cache_size);

void dumpLog(std::queue<std::vector<std::string>> &log);