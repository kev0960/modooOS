#ifndef TYPES_H
#define TYPES_H

#include "stdint.h"

namespace Kernel {

using size_t = unsigned long;
using off_t = long long;
using ptrdiff_t = long long;

static const size_t npos = -1;

}  // namespace Kernel
#endif
