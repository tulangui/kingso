#ifndef _MS_ROW_LOCALIZER_H_
#define _MS_ROW_LOCALIZER_H_
#include <stdint.h>
#include <stdio.h>

namespace is_assist {
    unsigned int strhash(const char *szBuf, int nLen);

    class RowLocalizer {
        public:
            RowLocalizer() { }
            virtual ~RowLocalizer() { }
        public:
            virtual int32_t init(const char *confpath) { return 0; }
            virtual uint32_t getLocatingSignal(const char * query, uint32_t size) {
                if(query==NULL || size <= 0) return 0;
                char* p = new char[size+1];
                if(p == NULL)
                    return 0;
                uint32_t ret = 0;
                size = conv(query, size, p);
                if(size > 0) {
                    ret = strhash(p, size) + 1;
                }
                delete[] p;
                return ret;	
            }
        private:
            int conv(const char* src, int src_len, char* dest);
    };

    class RowLocalizerFactory {
        public:
            static RowLocalizer * make(const char * name);
            static void recycle(RowLocalizer * res);
    };

}

#endif //_MS_ROW_LOCALIZER_H_
