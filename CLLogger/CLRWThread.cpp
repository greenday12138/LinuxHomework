#include "CLRWThread.h"
#include "CLFileRW.h"

CLReadThread::CLReadThread(int length) { rLength = length; }
CLReadThread::~CLReadThread() {}

CLStatus CLReadThread::RunThreadFunction() {
  CLStatus s = CLFileRW::FileRead(m_ReadContext, rLength);
  if (!s.IsSuccess()) {
    std::cerr << "read file error." << std::endl;
    return CLStatus(-1, 0);
  }
  std::cout << "read file success." << std::endl << m_ReadContext << std::endl;
  return CLStatus(0, 0);
}

CLWriteThread::CLWriteThread(char *wStr) { strcpy(m_WriteContext, wStr); }
CLWriteThread::~CLWriteThread() {}

CLStatus CLWriteThread::RunThreadFunction() {
  CLStatus s = CLFileRW::FileWrite(m_WriteContext);

  if (!s.IsSuccess()) {
    std::cerr << "write file error." << std::endl;
    return CLStatus(-1, 0);
  }
  std::cout << "write file success." << std::endl;
  return CLStatus(0, 0);
}
