#include <algorithm>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <queue>
#include <sstream>
#include <string>
#include <vector>

#include "Util.h"

namespace fs = std::filesystem;

std::string fileGen(std::string old_file, std::string new_file_name) {
  fs::path inputPath(old_file);
  fs::path parentPath = inputPath.parent_path();
  std::string extension = inputPath.extension();
  std::string newFileName = (parentPath / new_file_name).string() + extension;

  return newFileName;
}

void readFile(std::vector<char> &read_cache, std::vector<int64_t> &cache,
              std::ifstream &inFile, size_t size) {
  int64_t number;
  read_cache.resize(size);

  inFile.read(read_cache.data(), size);
  size_t bytesRead = inFile.gcount();

  // 将读取的数据解析成 int64_t 格式
  for (size_t i = 0; i + sizeof(int64_t) + sizeof(DELIMITER) <= bytesRead;
       i += sizeof(int64_t) + sizeof(DELIMITER)) {
    std::memcpy(&number, &read_cache[i], sizeof(int64_t));
    cache.push_back(number);
  }

  read_cache.clear();

  // 文本格式读取
  // while (inFile >> number && iter < size) {
  //   cache.push_back(number);
  //   ++iter;
  // }
  // std::string line;
  // while (std::getline(inFile, line) && iter < size) {
  //   std::istringstream lineStream(line);
  //   while (lineStream >> number && iter < size) {
  //     cache.push_back(number);
  //     ++iter;
  //   }
  // }
}

void mergeFile(std::string first, std::string second,
               std::queue<std::string> &que, std::mutex &que_mutex,
               std::queue<std::vector<std::string>> &log_queue,
               size_t cache_size) {
  // 获取文件路径和扩展名
  fs::path firstPath(first), secondPath(second);
  fs::path parentPath = firstPath.parent_path();
  std::string extension = firstPath.extension();

  // 生成新的文件名，以 _ 作为后缀
  std::string newFileName =
      (parentPath / (firstPath.stem().string() + "_")).string() + extension;

  // 打开输入的两个文件以二进制模式
  std::ifstream file1(first, std::ios::binary), file2(second, std::ios::binary);
  if (!file1 || !file2) {
    std::cerr << "无法打开文件：" << first << " 或 " << second << std::endl;
    return;
  }
  // 创建一个新的输出文件用于存放合并后的数据
  std::ofstream outFile(newFileName, std::ios::binary | std::ios::app);
  if (!outFile) {
    std::cerr << "无法创建文件：" << newFileName << std::endl;
    return;
  }

  // 计算缓存区大小，确保为 int64_t 大小 + 分隔符的倍数
  size_t size =
      (size_t)(cache_size * 1024.0 / (sizeof(int64_t) + sizeof(DELIMITER))) /
      2 * (sizeof(int64_t) + sizeof(DELIMITER));

  // 定义缓存区
  std::vector<int64_t> cache1, cache2; // 用于存储读取的数据
  std::vector<char> write_cache, read_cache1,
      read_cache2;             // 用于写入和读取的数据
  size_t iter1 = 0, iter2 = 0; // 缓存读取的指针
  size_t size1 = 0, size2 = 0; // 缓存区大小

  do {
    // 如果缓存1已读完，从文件1中读取新的数据
    if (iter1 == size1) {
      cache1.clear();
      // 从文件1读取数据到缓存
      readFile(read_cache1, cache1, file1, size);
      iter1 = 0;
      size1 = cache1.size(); // 更新缓存区大小

      outFile.write(reinterpret_cast<const char *>(write_cache.data()),
                    write_cache.size()); // 将已缓存的数据写入文件
      write_cache.clear();               // 清空写入缓存
    }
    // 如果缓存2已读完，从文件2中读取新的数据
    if (iter2 == size2) {
      cache2.clear();
      // 从文件2读取数据到缓存
      readFile(read_cache2, cache2, file2, size);
      iter2 = 0;
      size2 = cache2.size(); // 更新缓存区大小

      outFile.write(reinterpret_cast<const char *>(write_cache.data()),
                    write_cache.size()); // 将已缓存的数据写入文件
      write_cache.clear();               // 清空写入缓存
    }

    // 如果两个缓存都为空，结束合并过程
    if (size1 == 0 && size2 == 0) {
      break;
    } else if (size1 != 0 && size2 != 0) {
      // 如果两个缓存都有数据，比较并合并
      if (cache1[iter1] < cache2[iter2]) {
        // 将 cache1 中的数据转为字节并加入缓存
        write_cache.insert(
            write_cache.end(), reinterpret_cast<const char *>(&cache1[iter1]),
            reinterpret_cast<const char *>(&cache1[iter1]) + sizeof(int64_t));
        write_cache.push_back(DELIMITER); // 用分隔符分隔每个数字
        ++iter1;
      } else if (cache1[iter1] > cache2[iter2]) {
        // 将 cache2 中的数据转为字节并加入缓存
        write_cache.insert(
            write_cache.end(), reinterpret_cast<const char *>(&cache2[iter2]),
            reinterpret_cast<const char *>(&cache2[iter2]) + sizeof(int64_t));
        write_cache.push_back(DELIMITER);
        ++iter2;
      } else {
        // 如果两个数字相等，两个都保留
        write_cache.insert(
            write_cache.end(), reinterpret_cast<const char *>(&cache1[iter1]),
            reinterpret_cast<const char *>(&cache1[iter1]) + sizeof(int64_t));
        write_cache.push_back(DELIMITER);
        write_cache.insert(
            write_cache.end(), reinterpret_cast<const char *>(&cache2[iter2]),
            reinterpret_cast<const char *>(&cache2[iter2]) + sizeof(int64_t));
        write_cache.push_back(DELIMITER);
        ++iter1;
        ++iter2;
      }
    } else if (size1 == 0) {
      // 如果只有 cache2 有数据
      write_cache.insert(
          write_cache.end(), reinterpret_cast<const char *>(&cache2[iter2]),
          reinterpret_cast<const char *>(&cache2[iter2]) + sizeof(int64_t));
      write_cache.push_back(DELIMITER);
      ++iter2;
    } else {
      // 如果只有 cache1 有数据
      write_cache.insert(
          write_cache.end(), reinterpret_cast<const char *>(&cache1[iter1]),
          reinterpret_cast<const char *>(&cache1[iter1]) + sizeof(int64_t));
      write_cache.push_back(DELIMITER);
      ++iter1;
    }
  } while (iter1 < size1 || iter2 < size2); // 继续合并，直到两个缓存都为空

  // 关闭所有文件流
  file1.close();
  file2.close();
  outFile.close();

  // 删除合并后的源文件
  try {
    fs::remove(first);
    fs::remove(second);
  } catch (const std::exception &e) {
    std::cerr << "删除文件失败：" << e.what() << std::endl;
  }

  // 将合并后的新文件重命名为第一个文件的名字
  fs::rename(newFileName, first);
  newFileName = first;

  // 将文件名和合并记录加入队列，供其他线程处理
  {
    std::lock_guard<std::mutex> lock(que_mutex);
    que.push(newFileName);
    log_queue.push({fs::path(newFileName).stem().string(),
                    firstPath.stem().string(), secondPath.stem().string()});
  }
}

std::string sortFile(std::string filename) {
  // 为了加速后续文件合并过程，sortFile会将输出文件转换为二进制格式
  std::ifstream input_file(filename, std::ios::binary); // 以二进制模式打开文件
  if (!input_file) {
    std::cerr << "无法打开文件：" << filename << std::endl;
    return filename;
  }

  input_file.seekg(0, std::ios::end); // 定位到文件末尾，获取文件大小
  size_t buffer_size = input_file.tellg(); // 获取文件大小
  input_file.seekg(0, std::ios::beg);      // 定位回文件开头

  std::vector<int64_t> data; // 存储文件中的数据
  int64_t number;            // 用于读取每个数字

  // 使用缓冲区进行批量读取，加快读取速度
  std::string buff;
  buff.resize(buffer_size); // 预先为读取的数据分配内存
  input_file.read(&buff[0], buffer_size); // 批量读取文件内容
  std::istringstream ssin(buff); // 使用字符串流将文件内容转为可处理的流
  while (ssin >> number) { // 逐个读取64位整数并存储到vector中
    data.push_back(number);
  }
  input_file.close();

  std::sort(data.begin(), data.end()); // 对读取的数据进行排序

  // 以二进制模式打开输出文件，清空原文件内容
  std::ofstream output_file(filename, std::ios::binary | std::ios::trunc);
  if (!output_file) {
    std::cerr << "无法打开文件进行写入：" << filename << std::endl;
    return filename;
  }

  // 使用缓冲区进行批量写入，加快写入速度
  // 文本格式输出方式, 用于DEBUG
  // std::ostringstream buffer;
  // for (const auto &number : data) {
  //   buffer << number << DELIMITER;  // 用分隔符分隔每个数字
  // }
  // output_file << buffer.str();  // 输出字符串流内容
  // output_file.close();

  // 二进制文件输出格式
  std::vector<char> cache; // 用于存储写入文件的二进制数据
  for (const auto &number : data) {
    // 将每个整数转换为二进制并添加到缓存中
    cache.insert(cache.end(), reinterpret_cast<const char *>(&number),
                 reinterpret_cast<const char *>(&number) + sizeof(int64_t));
    cache.push_back(
        DELIMITER); // 为了方便debug, 在中间文件中加入文本文件类似的分隔符
  }
  output_file.write(cache.data(), cache.size()); // 将缓存中的数据写入文件
  output_file.close();

  return filename;
}

std::vector<std::string> splitFile(std::string filename, size_t file_size) {
  // 将一个文件按照指定大小切分成多个小文件，每个文件的大小不超过给定的
  // file_size（单位为 KB），输出返回的文件集；
  std::vector<std::string> file_parts;

  // 确保切分点是 int64_t 大小 + 分隔符的倍数
  file_size =
      (int)(file_size * 1024.0 / (sizeof(int64_t) + sizeof(DELIMITER))) *
      (sizeof(int64_t) + sizeof(DELIMITER));

  // 打开源文件
  std::ifstream input_file(filename, std::ios::binary);
  if (!input_file) {
    std::cerr << "无法打开文件: " << filename << std::endl;
    return file_parts;
  }

  // 获取文件大小
  input_file.seekg(0, std::ios::end);
  size_t total_size = input_file.tellg();
  input_file.seekg(0, std::ios::beg);

  size_t total_parts =
      (total_size + file_size - 1) / file_size; // 计算文件切分的数量

  std::filesystem::path inputPath(filename);

  // 切分文件并保存为多个文件
  for (size_t i = 0; i < total_parts; ++i) {
    // 新文件名
    std::string part_filename = fileGen(
        filename, inputPath.stem().string() + "_" + std::to_string(i + 1));
    file_parts.push_back(part_filename);

    // 打开输出文件
    std::ofstream output_file(part_filename, std::ios::binary);
    if (!output_file) {
      std::cerr << "无法创建文件: " << part_filename << std::endl;
      return file_parts;
    }

    // 计算当前分段的大小
    size_t current_part_size =
        (i == total_parts - 1) ? (total_size - i * file_size) : file_size;

    // 读取源文件并写入输出文件
    std::vector<char> buffer(current_part_size);
    input_file.read(buffer.data(), current_part_size);
    output_file.write(buffer.data(), current_part_size);
    output_file.close();
  }

  input_file.close();
  return file_parts;
}

std::string bin2Text(std::string origin_file, size_t cache_size) {
  fs::path origin(origin_file);
  std::string newFileName = fileGen(origin_file, origin.stem().string() + "t");
  std::ifstream inFile(origin_file, std::ios::binary);
  std::ofstream outFile(newFileName, std::ios::trunc);
  cache_size =
      (size_t)(cache_size * 1024.0 / (sizeof(int64_t) + sizeof(DELIMITER))) *
      (sizeof(int64_t) + sizeof(DELIMITER));

  std::vector<char> buffer(cache_size);
  std::vector<int64_t> data;
  int64_t number;
  while (inFile.read(buffer.data(), buffer.size())) {
    size_t bytesRead = inFile.gcount();
    for (int i = 0; i + sizeof(int64_t) + sizeof(DELIMITER) <= bytesRead;
         i += sizeof(int64_t) + sizeof(DELIMITER)) {
      std::memcpy(&number, &buffer[i], sizeof(int64_t));
      data.push_back(number);
    }
  }

  std::ostringstream buff;
  for (size_t i = 0; i < data.size(); ++i) {
    buff << data[i] << DELIMITER;
    if (i != 0 && i % (cache_size * 1024 / sizeof(int64_t)) == 0) {
      outFile << buff.str();
    }
  }
  outFile << buff.str();

  outFile.close();
  inFile.close();

  return newFileName;
}

void dumpLog(std::queue<std::vector<std::string>> &log) {
  std::ofstream outFile("./merge.log", std::ios::trunc);
  std::ostringstream buffer;
  buffer << std::left << std::setw(20) << "Final File" << std::left
         << std::setw(20) << "First File" << std::left << std::setw(20)
         << "Second File" << "\n";

  while (!log.empty()) {
    std::vector<std::string> files = log.front();
    log.pop();
    buffer << std::left << std::setw(20) << files[0] << "\t";
    buffer << std::left << std::setw(20) << files[1] << "\t";
    buffer << std::left << std::setw(20) << files[2] << "\n";
  }

  outFile << buffer.str();
  outFile.close();
}