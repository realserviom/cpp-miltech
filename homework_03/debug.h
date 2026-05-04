#ifndef DEBUG_H
#define DEBUG_H

#include <iostream>
#include <iomanip>

#define ENABLE_LOG	1
#define ENABLE_DEBUG  1
 
#if ENABLE_LOG
  #define LOG(msg) std::cout << "[LOG] " << msg << std::endl
#else
  #define LOG(msg)
#endif
 
#if ENABLE_DEBUG
  #define DEBUG(msg) std::cout << "[DEBUG] " << msg << std::endl
#else
  #define DEBUG(msg)
#endif


#endif // DEBUG_H