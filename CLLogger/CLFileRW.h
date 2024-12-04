#pragma once

#include <memory>
#include <mutex>
#include <queue>

#include "CLStatus.h"

class CLFileRW {
public:
  static std::shared_ptr<CLFileRW> GetInstance();    // 获取文件操作对象
  static CLStatus FileWrite(const char *wMsg);       // 文件的写操作
  static CLStatus FileRead(char *rMsg, int rLength); // 文件的读操作

  CLStatus FWrite(const char *wMsg);
  CLStatus FRead(char *rMsg, int rLength);
  CLStatus Flush(); // 写入写缓存到文件中

private:
  CLFileRW();
  CLFileRW(const CLFileRW &) = delete;
  CLFileRW &operator=(const CLFileRW &) = delete;
  ~CLFileRW();

  int m_Fd;                            // 文件标识符
  pthread_mutex_t *m_pMutexForUseFile; // 文件使用互斥量

  std::queue<std::unique_ptr<char[]>> write_queue_; // 写队列
  std::unique_ptr<char[]> read_buffer_;  // 读缓存

  static std::shared_ptr<CLFileRW> instance_; // 文件操作对象的实例
  static std::mutex mutex_for_creating_file_; // 创建文件互斥量
  static std::mutex mutex_for_write_;         // 使用文件互斥量
  static std::mutex mutex_for_read_;          // 读互斥量
  static std::mutex mutex_for_use_file_;      // 使用文件互斥量

  static const std::string LOG_FILE_NAME_; // 文件名
  static const int BUFFER_SIZE_;           // 缓存大小
};
