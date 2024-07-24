#pragma once

#ifdef USE_PTHREAD

#include "lock/pthread.h"

typedef LockObjectPthread LockFast;
typedef LockObjectPthread Lock;

#else /* USE_PTHREAD */

#include "lock/simple.h"
#include "lock/brute.h"

typedef LockObjectBrute<> LockFast;
typedef LockObjectSimple Lock;

#endif /* USE_PTHREAD */
