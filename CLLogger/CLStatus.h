#pragma once

#include <cstring>
#include <iostream>

// 用于保存函数的处理结果
class CLStatus {
public:
  CLStatus(long lReturnCode, long lErrorCode)
      : return_code_(lReturnCode), error_code_(lErrorCode) {}

  CLStatus(const CLStatus &s) {
    if (&s == this)
      return;
    return_code_ = s.return_code_;
    error_code_ = s.error_code_;
  }

  virtual ~CLStatus(){};

  bool IsSuccess() {
    // lReturnCode >=0表示成功，否则失败
    if (return_code_ >= 0)
      return true;
    else
      return false;
  }

  long ReturnCode() const { return return_code_; }
  long ErrorCode() const { return error_code_; }

private:
  long return_code_;
  long error_code_;
};