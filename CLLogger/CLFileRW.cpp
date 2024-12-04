#include "CLFileRW.h"
#include "CLStatus.h"
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <memory>
#include <mutex>
#include <unistd.h>

const std::string CLFileRW::LOG_FILE_NAME_ = "temp.txt";
const int CLFileRW::BUFFER_SIZE_ = 4096;
std::shared_ptr<CLFileRW> CLFileRW::instance_ = nullptr;
std::mutex CLFileRW::mutex_for_creating_file_;
std::mutex CLFileRW::mutex_for_use_file_;
std::mutex CLFileRW::mutex_for_write_;
std::mutex CLFileRW::mutex_for_read_;

CLFileRW::CLFileRW() {
  m_Fd = open(LOG_FILE_NAME_.c_str(), O_RDWR | O_CREAT | O_APPEND,
              S_IRUSR | S_IWUSR);
  if (m_Fd == -1)
    throw "In CLFileRW::CLFileRW(), open error";
}

CLFileRW::~CLFileRW() {
  // 程序退出时，需要自动刷新缓存并关闭文件
  Flush();
  close(m_Fd);
}

CLStatus CLFileRW::FileWrite(const char *wMsg) {
  std::shared_ptr<CLFileRW> pFile = CLFileRW::GetInstance(); // 获取文件操作对象
  CLStatus s = pFile->FWrite(wMsg);
  if (s.IsSuccess())
    return CLStatus(0, 0);
  else
    return CLStatus(-1, 0);

  // if (auto pFile = instance_.lock()) {
  //   CLStatus s = pFile->FWrite(wMsg);
  //   if (s.IsSuccess())
  //     return CLStatus(0, 0);
  //   else
  //     return CLStatus(-1, 0);
  // } else {
  //   return CLStatus(-1, 0);
  // }
}

CLStatus CLFileRW::FWrite(const char *wMsg) {

  if (wMsg == 0)
    return CLStatus(-1, 0);
  int len_strmsg = strlen(wMsg);

  if (len_strmsg == 0)
    return CLStatus(-1, 0);

  {
    std::lock_guard<std::mutex> lock(mutex_for_write_);
    auto temp = std::unique_ptr<char[]>(new char[len_strmsg+1]);
    memcpy(temp.get(), wMsg, len_strmsg);
    write_queue_.push(std::move(temp));
  }

  if (write_queue_.size() == 1)
    return Flush();
  else {
    return CLStatus(0, 0);
  }
}

CLStatus CLFileRW::Flush() {
  std::lock_guard<std::mutex> file_lock(mutex_for_use_file_);

  while (!write_queue_.empty()) {
    int offset = lseek(m_Fd, 0, SEEK_END);
    const char *buffer = write_queue_.front().get();
    if (write(m_Fd, buffer, strlen(buffer)) == -1) {
      return CLStatus(-1, errno);
    }
    write_queue_.pop();
  }

  return CLStatus(0, 0);
}

std::shared_ptr<CLFileRW> CLFileRW::GetInstance() {
  // 只创建一次文件操作对象
  if (instance_ == nullptr) {
    std::lock_guard<std::mutex> lock(mutex_for_creating_file_);
    if (instance_ == nullptr) {
      instance_ = std::shared_ptr<CLFileRW>(new CLFileRW(),
                                            [](CLFileRW *ptr) { delete ptr; });
    }
  }

  return instance_;
}

CLStatus CLFileRW::FileRead(char *rMsg, int rLength) {
  std::shared_ptr<CLFileRW> pFile = CLFileRW::GetInstance(); // 获取文件操作对象
  if (pFile == 0)
    return CLStatus(-1, 0);

  if (rMsg == 0) {
    return CLStatus(-1, 0);
  }
  if (rLength == 0) {
    return CLStatus(0, 0);
  }

  CLStatus s = pFile->FRead(rMsg, rLength);
  if (s.IsSuccess())
    return CLStatus(0, 0);
  else
    return CLStatus(-1, 0);
}

CLStatus CLFileRW::FRead(char *rMsg, int rLength) {

  if (rLength == 0)
    return CLStatus(0, 0);

  std::lock_guard<std::mutex> read_lock(mutex_for_read_);
  if (read_buffer_ == nullptr || strlen(read_buffer_.get()) < rLength) {
    std::lock_guard<std::mutex> file_lock(mutex_for_use_file_);

    read_buffer_ = std::unique_ptr<char[]>(new char[4098]);
    lseek(m_Fd, 0, SEEK_SET);
    if (read(m_Fd, read_buffer_.get(), rLength) == -1)
      return CLStatus(-1, 0);
    std::cout << "read a lot, read from file." << std::endl; // 提示信息
  }

  memcpy(rMsg, read_buffer_.get(), rLength);
  std::cout << "read from buffer." << std::endl; // 提示信息

  return CLStatus(0, 0);
}