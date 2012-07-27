#include "util/Postcode.h"
#include <string.h>

struct ProCodePair
{
    const char * province;
    const char * postcode;
    int shortlen;
};

static ProCodePair PRO_CODE_PAIR[] = {
    {"北京市","110000",6},           {"天津市","120000",6},
    {"河北省","130000",6},           {"山西省","140000",6},
    {"内蒙古自治区","150000",6},     {"辽宁省","210000",6},
    {"吉林省","220000",6},           {"黑龙江省","230000",6},
    {"上海市","310000",6},           {"江苏省","320000",6},
    {"浙江省","330000",6},           {"安徽省","340000",6},
    {"福建省","350000",6},           {"江西省","360000",6}, 
    {"山东省","370000",6},           {"河南省","410000",6},
    {"湖北省","420000",6},           {"湖南省","430000",6},
    {"广东省","440000",6},           {"广西壮族自治区","450000",6},
    {"海南省","460000",6},           {"重庆市","500000",6},
    {"四川省","510000",6},           {"贵州省","520000",6},
    {"云南省","530000",6},           {"西藏自治区","540000",6},
    {"陕西省","610000",6},           {"甘肃省","620000",6},
    {"青海省","630000",6},           {"宁夏回族自治区","640000",6},
    {"新疆维吾尔自治区","650000",6}, {"台湾省","710000",6},
    {"香港特别行政区","810000",6},   {"澳门特别行政区","820000",6},
    {"海外","990000",6}
};

const char * Postcode::convert(const char * province)
{
    static int num = sizeof(PRO_CODE_PAIR) / sizeof(ProCodePair);
    if(!province) {
        return NULL;
    }
    for(int i = 0; i < num; i++) {
        if(strncmp(PRO_CODE_PAIR[i].province, province, 
                    PRO_CODE_PAIR[i].shortlen) == 0)
        {
            return PRO_CODE_PAIR[i].postcode;
        }
    }
    return NULL;
}
