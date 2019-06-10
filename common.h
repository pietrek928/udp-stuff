#ifndef __COMMON_H_
#define __COMMON_H_

#define __STR(x) #x
#define STR(x) __STR(x)
#define __AT__ __FILE__ ":" STR(__LINE__)

#define LIKELY(condition) __builtin_expect(static_cast<bool>(condition), 1)
#define UNLIKELY(condition) __builtin_expect(static_cast<bool>(condition), 0)


typedef uint8_t byte_t;

#endif /* __COMMON_H_ */

