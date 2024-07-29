#pragma once

#ifdef USE_PTHREAD

#include "lock/pthread.h"

typedef LockObjectPthread LockFrequent;
typedef LockObjectPthread Lock;

#else /* USE_PTHREAD */

#include "lock/simple.h"
#include "lock/brute.h"

typedef LockObjectBrute<> LockFrequent;
typedef LockObjectSimple Lock;

#endif /* USE_PTHREAD */
