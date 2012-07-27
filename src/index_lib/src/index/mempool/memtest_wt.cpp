#include "ShareMemPool.h"

int main(int argc, char * argv[])
{
    if (argc != 2) {
        printf("usage:\n\t%s [path] \n", argv[0]);
        return -1;
    }
	index_mem::ShareMemPool mempool;
	int ret = mempool.create(argv[1]);
	if (ret != 0) {
		printf("create mempool err\n");
		return -1;
	}

    u_int stri = 0;
    char * str = NULL;
    int   size = 0;

    for(int pos = 0; pos < 1; pos++) {
        stri = mempool.alloc(1024);
        str = (char *)mempool.addr(stri);
        size = mempool.size(stri);
        snprintf(str, size, "%s %d", "hello world", pos);
    }

    printf("len=%d str=%s key=%u\n", size, str, stri);
    mempool.dealloc(stri);

    stri = mempool.alloc(100000);
	str = (char *)mempool.addr(stri);
	size = mempool.size(stri);
	snprintf(str, size, "%s", "hehe,pujian!");
	printf("len=%d str=%s key=%u\n", size, str, stri);
    /*
    mempool.reset();
    stri = mempool.alloc(100000);
	str = (char *)mempool.addr(stri);
	size = mempool.size(stri);
	snprintf(str, size, "%s", "hehe,pujian!");
	printf("len=%d str=%s key=%u\n", size, str, stri);
    */
    sleep(60);
    mempool.dump(argv[1]);
    //mempool.destroy();
    mempool.detach();
    return 0;
}
