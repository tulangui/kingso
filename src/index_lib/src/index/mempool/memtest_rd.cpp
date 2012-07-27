#include "ShareMemPool.h"

int main(int argc, char * argv[])
{
    if (argc != 3) {
        printf("%s\t [path] [key]\n", argv[1]);
        return -1;
    }

	index_mem::ShareMemPool mempool;
	int ret = mempool.loadData(argv[1]);
	if (ret != 0) {
		printf("load mempool err\n");
		return -1;
	}

    u_int key = atoi(argv[2]);
	char *str = (char *)mempool.addr(key);
	int size = mempool.size(key);
	printf("len=%d str=%s key=%u\n", size, str, key);
    mempool.destroy();
    //mempool.detach();
    return 0;
}
