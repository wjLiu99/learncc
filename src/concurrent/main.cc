#include "synclog.h"
#define LOG_LEVEL DEBUG
extern void ring_queue_test();
extern void ulring_queue_test();
extern void ulring_queue_test1();
extern void ex_test();
extern void queue_test();
extern void threadsafe_queue_l_test();
extern void hash_test();
int main(int argc, char const *argv[])
{
    hash_test ();

    return 0;
}
