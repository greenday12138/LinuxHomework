#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <string>

#include "ThreadPool.h"
#include "Util.h"

namespace fs = std::filesystem;

// 64位有符号整数的范围定义
const int64_t MIN_INT64 = INT64_MIN;
const int64_t MAX_INT64 = INT64_MAX;

// 目标总数据量设置为1GB
size_t TOTAL_DATA_SIZE = 1 * 1024 * 1024 * 1024;

// 每个文件的大小范围（字节），最小50KB，最大50MB
const size_t MIN_FILE_SIZE = 50 * 1024;
const size_t MAX_FILE_SIZE = 50 * 1024 * 1024;

// 通过给定的随机数生成器和分布生成一个随机整数
int64_t generate_random_int64(std::mt19937_64 &rng,
                              std::uniform_int_distribution<int64_t> &dist) {
  return dist(rng);
}

void generate_files(const std::string &output_dir) {
  // 创建输出目录，如果目录已存在则不会重复创建
  fs::create_directories(output_dir);

  // 随机数生成器初始化
  std::random_device rd;
  std::mt19937_64 rng(rd()); // 使用64位的随机数生成器
  std::uniform_int_distribution<int64_t> data_dist(
      MIN_INT64, MAX_INT64); // 随机生成64位有符号整数
  std::uniform_int_distribution<size_t> file_size_dist(
      MIN_FILE_SIZE, MAX_FILE_SIZE); // 随机生成文件大小

  size_t generated_data_size = 0; // 已生成的数据大小
  size_t file_index = 1;          // 文件索引，从1开始

  // 当已生成的数据量小于目标总数据量时，继续生成文件
  while (generated_data_size < TOTAL_DATA_SIZE) {
    // 生成当前文件大小
    size_t current_file_size = file_size_dist(rng);
    size_t remaining_size =
        TOTAL_DATA_SIZE - generated_data_size; // 计算剩余数据量
    current_file_size = std::min(current_file_size,
                                 remaining_size); // 确保文件不会超过剩余数据量

    // 构建文件路径
    std::string file_path =
        output_dir + "/file_" + std::to_string(file_index++) + ".txt";

    // 打开文件，如果文件无法打开则输出错误信息并返回
    std::ofstream file(file_path);
    if (!file.is_open()) {
      std::cerr << "Failed to open file: " << file_path << std::endl;
      return;
    }

    // 向文件中写入随机数据
    size_t written_size = 0; // 已写入的字节数
    while (written_size < current_file_size) {
      // 生成一个随机整数并转换为字符串
      int64_t random_value = generate_random_int64(rng, data_dist);
      std::string data =
          std::to_string(random_value) + "\n"; // 每个整数后加换行符
      file << data;                            // 写入文件
      written_size += data.size();             // 更新已写入的字节数
    }

    // 关闭文件
    file.close();

    // 更新已生成的数据量
    generated_data_size += current_file_size;

    // 输出当前生成的文件信息
    std::cout << "Generated " << file_path << " (" << current_file_size
              << " bytes)" << std::endl;
  }

  // 输出所有文件生成完毕的信息
  std::cout << "All files generated. Total data size: " << generated_data_size
            << " bytes." << std::endl;
}

// find ./test -type f -newermt 2024-12-05 -exec rm {} +

int main(int argc, char *argv[]) {
  // 主函数的第一个参数为输出目录，默认是"./test"
  // 第二个参数为目标数据大小（单位GB），用于生成数据文件

  // 如果提供了第一个参数则使用它作为输出目录
  std::string output_dir = argc > 1 ? argv[1] : "./test";
  // 如果提供了第二个参数，则使用它来调整目标数据大小（单位GB）
  double mul = argc > 2 ? std::stod(argv[2]) : 1.0;
  TOTAL_DATA_SIZE = static_cast<size_t>(mul * 1024 * 1024 *
                                        1024); // 计算目标数据量，单位为字节

  // 如果输出目录已存在，先删除它并重新创建
  if (std::filesystem::exists(output_dir)) {
    std::filesystem::remove_all(output_dir); // 删除已有的目录及其内容
  }
  std::filesystem::create_directories(output_dir); // 创建新的输出目录

  // 调用generate_files函数生成数据文件
  generate_files(output_dir);

  //   std::vector<char> data;
  //   int64_t num = -7006416813515049408;
  //   data.insert(data.end(), reinterpret_cast<const char *>(&num),
  //               reinterpret_cast<const char *>(&num) + sizeof(int64_t));
  //   data.push_back(DELIMITER); // 添加换行符
  //   std::ofstream out("./temp.txt", std::ios::binary | std::ios::app);
  //   out.write(data.data(), data.size());
  //   out.close();
  //   // 读取文件中的二进制数据
  //   std::ifstream in("./temp.txt");
  //   std::vector<char> readData((std::istreambuf_iterator<char>(in)),
  //                              std::istreambuf_iterator<char>());
  //   in.close();

  //   // 按字节大小恢复数字
  //   for (size_t i = 0; i < readData.size();
  //        i += sizeof(int64_t) + sizeof(DELIMITER)) {
  //     int64_t restoredNum;
  //     char delimiter;
  //     // 将读取的字节流恢复为int64_t类型
  //     std::memcpy(&restoredNum, &readData[i], sizeof(int64_t));
  //     std::cout << restoredNum << std::endl;
  //   }

  //   std::ifstream in("./temp.txt", std::ios::binary);
  //   std::string line;
  //   while (getline(in, line)) {
  //     std::istringstream ss(line);
  //     while (ss >> num) {
  //       std::cout << num << std::endl;
  //     }
  //   }
  //   in.close();

  return 0;
}