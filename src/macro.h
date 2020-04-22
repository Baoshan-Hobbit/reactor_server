/*************************************************************************
  > File Name: macro.h
  > Author: baoshan
  > Mail: baoshanw@foxmail.com 
  > Created Time: 2020年03月18日 星期三 09时26分10秒
 ************************************************************************/
#ifndef CPLUS_PRACTICE_MACRO_H_
#define CPLUS_PRACTICE_MACRO_H_

// macro 难以调试,如果名字写错会报错ISO C++ forbids declaration of *** with 
// no type,没半毛钱关系
// 尤其要注意把参数写上,经常忘记写
#define DISALLOW_COPY_AND_ASSIGN(ClassName) \
  ClassName(const ClassName&) = delete; \
  ClassName& operator=(const ClassName&) = delete

#endif // CPLUS_PRACTICE_MACRO_H_
