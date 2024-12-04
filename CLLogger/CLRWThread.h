#pragma once

#include "CLStatus.h"
#include "CLThread.h"

class CLReadThread : public CLThread {
public:
  CLReadThread(int length);
  virtual ~CLReadThread();
  virtual CLStatus RunThreadFunction();

private:
  char m_ReadContext[20];
  int rLength;
};

class CLWriteThread : public CLThread {
public:
  CLWriteThread(char *wStr);
  virtual ~CLWriteThread();
  virtual CLStatus RunThreadFunction();

private:
  char m_WriteContext[20];
};
