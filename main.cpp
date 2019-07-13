#include "gc_pointer.h"
#include "LeakTester.h"

int main()
{
    Pointer<int> p = new int(21);
    p = new int(24);
    p = new int(32);

    return 0;
}
