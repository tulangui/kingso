#include "util/PriorityQueue.h"
#include <stdio.h>
#include <stdint.h>

using namespace util;

struct SLess {
    inline bool operator() (int &a, int &b) {
        if (_porder) {
            return a >= b;
        }
        else {
            return a <= b;
        }
    }

    void setOrder(bool bPrimaryOrder, bool bSecondOrder) {
        _porder = bPrimaryOrder;
        _sorder = bSecondOrder;
    }

    bool _porder;
    bool _sorder;
};

int
main(int argc, char *argv[]) {

    SLess l;
    l.setOrder(false, true);

    PriorityQueue < int, SLess > queue(l);
    queue.initialize(5);
    queue.push(1);
    queue.push(2);
    queue.push(3);
    queue.push(4);
    queue.push(3);
    queue.push(2);
    queue.push(1);

    int out = 0;

    for (int i=0; i<10; i++) {
//        out = queue.pop();
        out = queue.doPopAndPush(1);
        printf("out=%d\n", out);
    }



    return 0;

}
