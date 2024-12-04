#include <chrono>
#include <filesystem>
#include <iostream>
#include <queue>

#include "ThreadPool.h"
#include "Util.h"

namespace fs = std::filesystem;

// 主程序
int main(int argc, char *argv[]) {
  // 主函数第一个参数为要处理的文件夹，默认为当前文件夹 "./"
  // 第二个参数为缓存空间的大小，单位为KB，默认为 512KB
  std::string inputDir = argc > 1 ? argv[1] : "./";
  const char *size = argc > 2 ? argv[2] : "512";
  size_t cache_size = static_cast<size_t>(std::stoll(size));

  // 创建一个线程池，线程数量根据硬件的核心数自动调整
  ThreadPool pool(std::thread::hardware_concurrency());
  std::queue<std::string> file_que;

  // 记录处理开始时间
  auto start = std::chrono::high_resolution_clock::now();

  // 使用线程池将目录下的所有文件拆分到可一次性读入内存的大小
  std::vector<std::future<std::vector<std::string>>> futures;
  for (const auto &entry : fs::directory_iterator(inputDir)) {
    if (entry.is_directory())
      continue; // 如果是目录则跳过

    const auto &inputFile = entry.path().string();
    // 将拆分任务添加到线程池
    futures.emplace_back(pool.enqueue(splitFile, inputFile, cache_size));
  }

  // 等待所有拆分任务完成，并将拆分后的文件路径加入文件队列
  for (auto &future : futures) {
    std::vector<std::string> tempVec = future.get();
    for (auto &&item : tempVec) {
      file_que.push(std::move(item));
    }
  }
  futures.clear(); // 清空任务列表

  // 打印拆分文件的处理时间
  std::queue<std::string> temp_que = file_que;
  while (!temp_que.empty()) {
    // 输出每个拆分后的文件名（可以用于调试）
    temp_que.pop();
  }
  auto end = std::chrono::high_resolution_clock::now();
  std::cout << "Split file time: "
            << std::chrono::duration_cast<std::chrono::milliseconds>(end -
                                                                     start)
                       .count() / 1000.0
            << "s" << std::endl;

  // 记录排序阶段的开始时间
  start = std::chrono::high_resolution_clock::now();

  // 使用线程池对拆分后的每个文件进行排序
  std::vector<std::future<std::string>> futures_1;
  while (!file_que.empty()) {
    // 将排序任务添加到线程池
    futures_1.emplace_back(pool.enqueue(sortFile, file_que.front()));
    file_que.pop();
  }

  // 等待排序完成，将排序后的文件路径重新加入队列
  for (auto &&future : futures_1) {
    file_que.push(std::move(future.get()));
  }
  futures_1.clear(); // 清空任务列表

  // 打印排序文件的处理时间
  end = std::chrono::high_resolution_clock::now();
  std::cout << "Sort file time: "
            << std::chrono::duration_cast<std::chrono::milliseconds>(end -
                                                                     start)
                       .count() /
                   1000.0
            << "s" << std::endl;

  // 记录合并阶段的开始时间
  start = std::chrono::high_resolution_clock::now();

  // 开始合并排序后的文件
  std::queue<std::string> &current_que = file_que;
  std::queue<std::vector<std::string>> mergeLog;
  std::mutex que_mutex;

  while (true) {
    {
      std::lock_guard<std::mutex> lock(que_mutex);
      // 如果队列中剩余文件小于2，且线程池没有正在运行的任务，说明合并完成
      if (current_que.size() < 2) {
        if (pool.finish())
          break; // 退出合并循环
      }
    }

    std::string firstFile = "", secondFile = "";
    {
      std::lock_guard<std::mutex> lock(que_mutex);
      // 从队列中取出两个文件进行合并
      if (!current_que.empty()) {
        firstFile = current_que.front();
        current_que.pop();
      }
    }
    {
      std::lock_guard<std::mutex> lock(que_mutex);
      if (!current_que.empty()) {
        secondFile = current_que.front();
        current_que.pop();
      }
    }

    // 如果某个文件为空，则重新加入队列
    if (firstFile == "" || secondFile == "") {
      std::lock_guard<std::mutex> lock(que_mutex);
      if (firstFile != "") {
        current_que.push(firstFile);
      }
      if (secondFile != "") {
        current_que.push(secondFile);
      }
    } else if (firstFile != secondFile) {
      // 否则将合并任务加入线程池
      pool.enqueue(mergeFile, firstFile, secondFile, std::ref(current_que),
                   std::ref(que_mutex), std::ref(mergeLog), cache_size);
    }
  }

  // 打印合并文件的处理时间
  end = std::chrono::high_resolution_clock::now();
  std::cout << "Merge file time: "
            << std::chrono::duration_cast<std::chrono::milliseconds>(end -
                                                                     start)
                       .count() / 1000.0
            << "s" << std::endl;

  // 将合并结果转换为文本文件
  std::string finalFile = current_que.front();
  bin2Text(finalFile, cache_size);
  fs::remove(finalFile); // 删除合并后的临时文件
  dumpLog(mergeLog);     // 输出合并日志

  return 0;
}
