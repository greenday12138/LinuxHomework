#include "CLRWThread.h"
#include "CLThread.h"
#include "unistd.h"
#include <memory>

int main() {
  char str[20] = "543210";
  char str1[20] = "zxcvbnm";
  std::shared_ptr<CLThread> wThread(new CLWriteThread(str));
  std::shared_ptr<CLThread> wThread1(new CLWriteThread(str1));
  std::shared_ptr<CLThread> rThread(new CLReadThread(6));
  std::shared_ptr<CLThread> rThread1(new CLReadThread(9));

  wThread->Run();
  sleep(1); // 先确保文件中已有数据，之后三个操作并行执行
  wThread1->Run();
  rThread1->Run();
  rThread->Run();

  wThread->WaitForDeath();
  wThread1->WaitForDeath();
  rThread->WaitForDeath();
  rThread1->WaitForDeath();

  return 0;
}
