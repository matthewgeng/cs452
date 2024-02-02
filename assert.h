#ifndef ASSERT_H
#define ASSERT_H

// assert
#define KASSERT(expr, str) \
    if (!(expr)) \
        assert(str) \

void assert(const char* str);

#endif
