/***********************************************************************************
 * Descri   : code transfer libary
 *          : translate ascii from lower<--> upper.
 *          : translate from quanjiao-->banjiao
 *          : translate from on encoding to another
 *
 * Author   : Paul Yang, zhenahoji@gmail.com
 *
 * Create   : 2009-04-17
 *
 * Update   : 2009-04-17
 **********************************************************************************/
#ifndef PY_CODE_H
#define PY_CODE_H

#include <wchar.h>

/*
 * func : translate an input string from upper case to lower case
 *
 * args : src, strlen, input string and its size.
 *      : desc, dsize, dest buffer and its size.
 *
 * ret  : 0, succeed;
 *      : -1, error.
 */
int py_2lower(const char* src, int srclen, char* dest, int dsize);

/*
 * func : translate an input string from lower case to upper case
 *
 * args : src, strlen, input string and its size.
 *      : desc, dsize, dest buffer and its size.
 *
 * ret  : 0, succeed;
 *      : -1, error.
 */
int py_2upper(const char* src, int srclen, char* dest, int dsize);

/*
 * func : translate an utf-8 buffer into unicode 16 buffer
 *
 * args : src, srclen, source buffer and its length
 *      : dest, dsize, dest buffer and its size
 *
 * ret  : -1, error
 *      : else, result buffer length
 */
int utf8_to_unicode(const char* src, int srclen, unsigned short* dest, int dsize);


/*
 * func : translate unicode 16 to utf-8
 *
 * args : src, srclen, unicode 16 buffer and its length
 *      : dest, dsize, dest utf-8 buffer and its size
 *
 * ret  : -1, error 
 *      : else, length of the result buffer
 */
int unicode_to_utf8(unsigned short* src, int srclen, char* dest, int dsize);

/*
 * func : translate a utf-8 buffer from quanjiao to banjiao
 *
 * args : src, srclen, source buffer and its length
 *      : dest, dsize, dest buffer and its size
 *
 * ret  : -1, error
 *      : else, length of the result buffer
 */

int py_2bj(const char* src, int srclen, char* dest, int dsize);

/*
 * func : translate a gbk buffer into unicode 16 buffer
 *
 * args : src, srclen, the source buffer and its length
 *      : dest, dsize, dest buffer and its size
 *
 * ret  : -1, error
 *      : else, length of the result buffer
 */
int py_gbk_to_unicode(const char* src, int srclen, unsigned short* dest, int dsize);


/*
 * func : translate a gbk buffer into utf-8 buffer
 *
 * args : src, srclen, source buffer and its length
 *      : dest, dsize, dest buffer and its size
 *
 * ret  : -1, error
 *      : else, length of the result buffer
 */
int py_gbk_to_utf8(const char* src, int srclen, char* dest, int dsize);

/*
 * func : translate a gbk buffer into utf-8 simplied
 *
 * args : src, srclen, source buffer and its length
 *      : dest, dsize, dest buffer and its size
 *
 * ret  : -1, error
 *      : else, length of the result buffer
 */
int py_gbk_to_utf8_s(const char* src, int srclen, char* dest, int dsize);

/*
 * func : translate a utf8 buffer into gbk buffer
 *
 * args : src, srclen, the source buffer and its length
 *      : dest, dsize, dest buffer and its size
 *
 * ret  : -1, error
 *      : else, length of the result buffer
 */
int py_utf8_to_gbk(const char* src, int srclen, char* dest, int dsize);


/*
 * func : translate a utf8 buffer into gbk buffer
 *
 * args : src, srclen, the source buffer and its length
 *      : dest, dsize, dest buffer and its size
 *
 * ret  : -1, error
 *      : else, length of the result buffer
 */
int py_utf8_to_gbk(const char* src, int srclen, char* dest, int dsize);


int py_code_init( const char* path );
void py_code_free(void);

#define GBK_HIMIN       0x81
#define GBK_HIMAX       0xfe
#define GBK_LOMIN       0x40
#define GBK_LOMAX       0xfe
#define IN_RANGE(ch, min, max)   ( (((unsigned char)(ch))>=(min)) && (((unsigned char)(ch))<=(max)) )
#define IS_GBK(cst)      ( IN_RANGE((cst)[0], GBK_HIMIN, GBK_HIMAX) && IN_RANGE((cst)[1], GBK_LOMIN, GBK_LOMAX) )

static const unsigned char s_head_byte_tab[256] =
{
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //0x00
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //0x10
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //0x20
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //0x30
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //0x40
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //0x50
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //0x60
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //1x70
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //1x80
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //1x90
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //1xA0
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //1xB9
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, //1xC0
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, //0xD0
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, //0xE0
	4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 6, 6  //0xF0
};

static const unsigned int s_utf8_head_mask_xor[] =
{
	0x00,                           /* unused */
	0x7F,                           /* 1 byte */
	0x1F,                           /* 2 bytes */
	0x0F,                           /* 3 bytes */
	0x07,                           /* 4 bytes */
	0x03,                           /* 5 bytes */
	0x01,                           /* 6 bytes */
};

static const unsigned int s_utf8_head_mask[] =
{
	0x00,                           // unused 
	0x00,                           // unused
	0xC0,                           // 2 bytes 110XXXXX
	0xE0,                           // 3 bytes 1110XXXX
	0xF0,                           // 4 bytes 11110XXX
	0xF8,                           // 5 bytes 111110XX
	0xFC,                           // 6 bytes 1111110X
};



/*
 * func : get a gbk word from a specified position in gbk buffer
 *
 * args : str,slen, input string and its length
 *      : pos, the specified position in str, after calling pos will step one word
 *
 * ret  : 0, reache the end of str
 *      : else, word value
 */
inline unsigned short get_gbk_word(const char* str, int slen, int *pos)
{

	unsigned short result = 0;
	unsigned char *tmpbuf=(unsigned char*)str;

	if(*pos == slen){
		return 0;
	}
	if((*pos + 1 < slen )&& (IS_GBK((unsigned char*)(tmpbuf+*pos))))
	{
		result=tmpbuf[*pos+1] *256 + tmpbuf[*pos];
		(*pos)+=2;
	}
	else{
		result=tmpbuf[*pos];
		(*pos)++;
	}
	return result;
}

/*
 * func : get a gbk word from a specified position in gbk buffer
 *
 * args : str,slen, input string and its length
 *      : pos, the specified position in str, after calling pos will step one word
 *
 * ret  : 0, reache the end of str
 *      : else, word value
 */
inline unsigned short get_utf8_word(const char* src, int slen, int *pos)
{

	unsigned char  ch  = src[*pos];
	unsigned int   len = s_head_byte_tab[ch];
	if( *pos + len > (unsigned int)slen  ){
		return 0;
	}

	unsigned short value = ch & s_utf8_head_mask_xor[len];
	for(unsigned int j=1;j<len;j++){
		ch = src[*pos+j];
		value = (value << 6) | (ch & 0x3F);
	}

	*pos += len;

	return value;

}

int py_utf8_normalize(const char* src, int srclen, char* dest, int dsize);
int py_utf8_to_s(const char* src, int srclen, char* dest, int dsize);

void py_unicode_normalize(wchar_t* str);


#endif


