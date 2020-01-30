#include "../std/printf.h"

#define DEFAULT_MOCK0 kprintf("%s is called\n", __FUNCTION__);
#define DEFAULT_MOCK1(ARG) \
  kprintf("%s  is called with " #ARG "=%lx\n", __FUNCTION__, ARG);
#define DEFAULT_MOCK2(ARG1, ARG2)                            \
  kprintf("%s is called with " #ARG1 "=%lx " #ARG2 "=%lx\n", \
          __FUNCTION__, ARG1, ARG2);
#define DEFAULT_MOCK3(ARG1, ARG2, ARG3)                                   \
  kprintf("%s is called with " #ARG1 "=%lx " #ARG2 "=%lx" #ARG3 "=%lx\n", \
          __FUNCTION__, ARG1, ARG2, ARG3);
