#pragma once

#include "CLStatus.h"
#include <pthread.h>

class CLThread {
public:
  CLThread();
  virtual ~CLThread();
  CLStatus Run();
  CLStatus WaitForDeath();

private:
  static void *StartFunctionOfThread(void *);

protected:
  virtual CLStatus RunThreadFunction() = 0;
  pthread_t m_ThreadID;
};
