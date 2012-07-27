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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <util/py_code.h>
#include <wchar.h>



extern unsigned short  g_gbk_uni_map[];
extern unsigned short  g_uni_gbk_map[];
extern unsigned short  g_uni_big5_map[];
extern unsigned short  g_big5_uni_map[];
extern unsigned short  g_uni_normal_map[];



// macros defined here
//

/*
 * func : translate an input string from upper case to lower case
 *
 * args : src, input string 
 *      : desc, dsize, dest buffer and its size.
 *
 * ret  : 0, succeed;
 *      : -1, error.
 *
 * note : dsize must be larger than srclen
 */
int py_2lower(const char* src, int srclen, char* dest, int dsize)
{
	int   dcnt = 0;
	static const unsigned char _lower_tab[256] = {
		0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
		16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
		32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
		48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
		64, 97, 98, 99,100,101,102,103,104,105,106,107,108,109,110,111,
		112,113,114,115,116,117,118,119,120,121,122, 91, 92, 93, 94, 95,
		96, 97, 98, 99,100,101,102,103,104,105,106,107,108,109,110,111,
		112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,
		128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,
		144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,
		160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,
		176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,
		192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,
		208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,
		224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,
		240,241,242,243,244,245,246,247,248,249,250,251,252,253,254,255,

	};


	assert( src && dest );
	if(dsize < srclen){
		return -1;
	}
	
	for(int i=0;i<srclen;i++){
		unsigned char ch = src[i];
		dest[dcnt++] = _lower_tab[ch];
	};

	dest[dcnt] = '\0';

	return dcnt;
}

/*
 * func : translate an input string from lower case to upper case
 *
 * args : src, strlen, input string and its size.
 *      : desc, dsize, dest buffer and its size.
 *
 * ret  : 0, succeed;
 *      : -1, error.
 *
 * note : dsize must be larger than srclen
 */
int py_2upper(const char* src, int srclen, char* dest, int dsize)
{
	int    dcnt = 0;
	static const unsigned char _upper_tab[256] = {
		0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
		16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
		32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
		48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
		64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
		80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95,
		96, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
		80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90,123,124,125,126,127,
		128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,
		144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,
		160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,
		176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,
		192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,
		208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,
		224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,
		240,241,242,243,244,245,246,247,248,249,250,251,252,253,254,255,
	};

	assert(src && dest);
	if(dsize<srclen){
		return -1;
	}

	for(int i=0;i<srclen;i++){
		unsigned char ch = src[i];
		dest[dcnt++] = _upper_tab[ch];
	}
	dest[dcnt] = '\0';

	return dcnt;

}


/*
 *
 */
inline int unicode_char_to_utf8(unsigned short value, char* buf, int bufsize)
{
	char word[6];
	int tail = sizeof(word)-2;

	word[sizeof(word)-1] = '\0';

	if(bufsize<1){
		return -1;
	}

	// deal with one byte ascii 
	if(value < 0x80){
		buf[0] = value;
		return 1;
	}
	// deal with multi byte word
	int j = tail;
	int len = 1;
	while(j>0){
		unsigned char ch = value & 0x3f;
		word[j--] = ch | 0x80;
		value = value>>6;
		++len;
		if(value<0x20){  // less than 6 bytes
			break;
		}
	}

	word[j] = value|s_utf8_head_mask[len];

	if(len>bufsize){
		fprintf(stderr, "dest buffer size too small [%d:%d]\n", len, bufsize);
		return -1;
	}
	memcpy(buf, word+j, len);
	return len;
}


/*
 * func : translate an utf-8 buffer into unicode 16 buffer
 *
 * args : src, srclen, source buffer and its length
 *      : dest, dsize, dest buffer and its size
 *
 * ret  : -1, error
 *      : else, result buffer length
 */
int utf8_to_unicode(const char* src, int srclen, unsigned short* dest, int dsize)
{
	int dcnt = 0;
	int pos  = 0;
	unsigned short word = 0;

    
	assert( src && dest );

        for( int i = 0; i < srclen; )
        {
		word = get_utf8_word(src, srclen, &pos);
		if(word==0){
			break;
		}

		dest[dcnt++] = word;
		if(dcnt==dsize){
			return -1;
		}

        }

        return dcnt;
}



/*
 * func : translate an input string from quanjiao to banjiao
 *
 * args : src, strlen, input string and its size.
 *      : desc, dsize, dest buffer and its size.
 *
 * ret  : -1, error.
 *      : else, length of the result buffer
 */
int py_gbk2bj(const char* src, int srclen, char* dest, int dsize)
{
	const unsigned char* p    = NULL;
	int   dcnt = 0;
	unsigned char ch = 0;

	assert(src&&dest&&dsize>srclen);

	p = (unsigned char*)src;
	while(*p != '\0'){
		if(*p>0x80 && *(p+1)!=0){
			
			if(*p == 0xA3 && *(p+1)>=0xA0){ // quanjiao alpha & number
				ch = *(p+1) - 0x80;
				dest[dcnt++] = ch;
				p += 2;
			}
			else{
				dest[dcnt++] = *p++;
				dest[dcnt++] = *p++;
			}

		}
		else{
			dest[dcnt++] = *p++;
		}
	}

	dest[dcnt] = '\0';

	return dcnt;
}


/*
 * func : translate an input string from quanjiao to banjiao
 *
 * args : src, strlen, input string and its size.
 *      : desc, dsize, dest buffer and its size.
 *
 * ret  : -1, error.
 *      : else, length of the result buffer
 */
int py_gbk2bj_mark(const char* src, int srclen, char* dest, int dsize)
{
	const unsigned char* p    = NULL;
	int   dcnt = 0;
	unsigned char ch = 0;

	static const unsigned char mark_tab[96] =
	{   0, ' ', ',', '.',  4,  5,  6,  7,  8,  9, '-', 11, 12, 13, '\'', '\'',
		'\"', '\"', 18, 19, 20, 21, 22, 23, '[', ']', '[', ']', 28, 29, '[', ']',
		32, 33, 34, ':', 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
		48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
		64, 65, 66, 67, 68, 69, 70, '$', 72, 73, 74, 75, 76, 77, 78, 79,
		80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95
	};


	assert(src&&dest);
	if(srclen>dsize){
		fprintf(stderr, "source lenght is larger than dest size\t"
		                           "error in %s\n", __FUNCTION__);
	}

	p   = (unsigned char*)src;
	while(*p != '\0'){
		if(*p>0x80 && *(p+1)!=0){
			
			if(*p == 0xA1 && *(p+1)>=0xA1){
				ch = *(p+1)-0xA1;
				if(mark_tab[ch]!=ch){
					dest[dcnt++] = mark_tab[ch]; // quanjiao mark 
					p += 2;
				}
			}
			else{
				dest[dcnt++] = *p++;
				dest[dcnt++] = *p++;
			}

		}
		else{
			dest[dcnt++] = *p++;
		}
	}

	dest[dcnt] = '\0';

	return dcnt;
}



/*
 * func : translate unicode 16 to utf-8
 *
 * args : src, srclen, unicode 16 buffer and its length
 *      : dest, dsize, dest utf-8 buffer and its size
 *
 * ret  : -1, error 
 *      : else, length of the result buffer
 */
int unicode_to_utf8(unsigned short* src, int srclen, char* dest, int dsize)
{
        char word[6];
	int dcnt = 0;
	int tail = 0;
	int len  = 0;

	assert(src && dest);

	word[sizeof(word)-1] = '\0';
	tail =sizeof(word)-2;

	for(int i=0; i<srclen; i++){
		unsigned short value = src[i];
		len = unicode_char_to_utf8(value, dest+dcnt, dsize-dcnt);
		if(len<0){
			return -1;
		}
		dcnt += len;

	}

	dest[dcnt] = '\0';

	return dcnt;
}

/*
 * func : translate a utf-8 buffer from quanjiao to banjiao
 *
 * args : src, srclen, source buffer and its length
 *      : dest, dsize, dest buffer and its size
 *
 * ret  : -1, error
 *      : else, length of the result buffer
 */
int py_2bj(const char* src, int srclen, char* dest, int dsize)
{
	int dcnt = 0;

	assert(dsize>=srclen);

	static const unsigned char _bj_tab1[256] = { // trans ａ～ｚ
		0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
		16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
		32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
		48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
		64, 97, 98, 99,100,101,102,103,104,105,106,107,108,109,110,111,
		112,113,114,115,116,117,118,119,120,121,122, 91, 92, 93, 94, 95,
		96, 97, 98, 99,100,101,102,103,104,105,106,107,108,109,110,111,
		112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,
		128, 97, 98, 99,100,101,102,103,104,105,106,107,108,109,110,111,
		112,113,114,115,116,117,118,119,120,121,122,155,156,157,158,159,
		160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,
		176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,
		192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,
		208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,
		224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,
		240,241,242,243,244,245,246,247,248,249,250,251,252,253,254,255,

	};

	static const unsigned char _bj_tab2[256] = { // trans Ａ～Ｚ，０～９
		0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
		16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
		32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
		48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
		64, 97, 98, 99,100,101,102,103,104,105,106,107,108,109,110,111,
		112,113,114,115,116,117,118,119,120,121,122, 91, 92, 93, 94, 95,
		96, 97, 98, 99,100,101,102,103,104,105,106,107,108,109,110,111,
		112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,
		128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,
		 48, 49, 50, 51, 52, 53, 54, 55, 56, 57,154,155,156,157,158,159,
		160, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
		 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91,188,189,190,191,
		192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,
		208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,
		224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,
		240,241,242,243,244,245,246,247,248,249,250,251,252,253,254,255,

	};


	for(int i=0;i<srclen;){

		if((unsigned char)src[i]==0xef){
			if((unsigned char)src[i+1]==0xbd){
				unsigned char ch = src[i+2];
				if(ch != _bj_tab1[ch]){
					dest[dcnt++] = _bj_tab1[ch];
					i += 3;

				}
				else{
					dest[dcnt++] = src[i++];
					dest[dcnt++] = src[i++];
					dest[dcnt++] = src[i++];
				}
			}
			else if((unsigned char)src[i+1]==0xbc){
				unsigned char ch = src[i+2];
				if(ch != _bj_tab2[ch]){
					dest[dcnt++] = _bj_tab2[ch];
					i += 3;
				}
				else{
					dest[dcnt++] = src[i++];
					dest[dcnt++] = src[i++];
					dest[dcnt++] = src[i++];
				}
			}
			else{
				dest[dcnt++] = src[i++];
				dest[dcnt++] = src[i++];
				dest[dcnt++] = src[i++];
			}
		}

		else{
			dest[dcnt++] = src[i++];
		}
	}

	dest[dcnt] = '\0';

	return 0;
}

/*
 * func : translate a gbk buffer into utf-8 buffer
 *
 * args : src, srclen, source buffer and its length
 *      : dest, dsize, dest buffer and its size
 *
 * ret  : -1, error
 *      : else, length of the result buffer
 */
int py_gbk_to_utf8(const char* src, int srclen, char* dest, int dsize)
{
	int pos = 0;
	int dcnt = 0;
	int len  = 0;
	unsigned short key = 0;
	unsigned short val = 0;

	assert(src && dest);

	key = get_gbk_word(src, srclen, &pos);
	while(key!=0){
		val = g_gbk_uni_map[key];
		len = unicode_char_to_utf8(val, dest+dcnt, dsize-dcnt);
		if(len<0){
			return -1;
		}
		dcnt += len;
		key = get_gbk_word(src, srclen, &pos);
	}

	dest[dcnt] = '\0';
	return dcnt;

}

/*
 * func : translate a gbk buffer into utf-8 buffer
 *
 * args : src, srclen, source buffer and its length
 *      : dest, dsize, dest buffer and its size
 *
 * ret  : -1, error
 *      : else, length of the result buffer
 */
/*
int py_gbk_to_utf8_s(const char* src, int srclen, char* dest, int dsize)
{
	int pos = 0;
	int dcnt = 0;
	int len  = 0;
	unsigned short key = 0;
	unsigned short val = 0;

	assert(src && dest);

	key = get_gbk_word(src, srclen, &pos);
	while(key!=0){
		val = g_gbk_unis_map[key];
		len = unicode_char_to_utf8(val, dest+dcnt, dsize-dcnt);
		if(len<0){
			return -1;
		}
		dcnt += len;
		key = get_gbk_word(src, srclen, &pos);
	}

	dest[dcnt] = '\0';
	return dcnt;

}
*/

/*
 * func : translate a gbk buffer into unicode 16 buffer
 *
 * args : src, srclen, the source buffer and its length
 *      : dest, dsize, dest buffer and its size
 *
 * ret  : -1, error
 *      : else, length of the result buffer
 */
int py_gbk_to_unicode(const char* src, int srclen, unsigned short* dest, int dsize)
{
	int   pos  = 0;
	int   dcnt = 0;
	int   key  = 0;
	unsigned short val  = 0;

	assert(src && dest);

	key = get_gbk_word(src, srclen, &pos);
	while(val!=0){
		val = g_gbk_uni_map[key];

		dest[dcnt++] = val;
		if(dcnt==dsize){
			return -1;
		}
	}
		
	return dcnt;
}




/*
 * func : translate a unicode16 buffer into gbk buffer
 *
 * args : src, srclen, the source buffer and its length
 *      : dest, dsize, dest buffer and its size
 *
 * ret  : -1, error
 *      : else, length of the result buffer
 */
int py_unicode_to_gbk(unsigned short* src, int srclen, char* dest, int dsize)
{
	int dcnt = 0;

	assert(src && dest);


	for(int i=0; i<srclen;i++){
		unsigned short key = 0;
		unsigned short val = 0;
		
		key = src[i];
		val = g_uni_gbk_map[key];

		dest[dcnt++] = val & 0xff;
		val = val>>8;
		if(val>0){
			dest[dcnt++] = val;
		}
		
		if(dcnt+1>=dsize){
			return -1;
		}
	}
	dest[dcnt] = '\0';
		
	return dcnt;
}

/*
 * func : translate a utf8 buffer into gbk buffer
 *
 * args : src, srclen, the source buffer and its length
 *      : dest, dsize, dest buffer and its size
 *
 * ret  : -1, error
 *      : else, length of the result buffer
 */
int py_utf8_to_gbk(const char* src, int srclen, char* dest, int dsize)
{
	int dcnt = 0;
	int pos  = 0;
	unsigned short word = 0;

	for( int i = 0; i < srclen; )
        {
		word = get_utf8_word(src, srclen, &pos);
		if(word==0){
			break;
		}

		word = g_uni_gbk_map[word];

		if(word<=127){
			dest[dcnt++] = word;
		}
		else{
			dest[dcnt++] = word & 0xff;
			word = word>>8;
			if(word>0){
				dest[dcnt++] = word;
			}
		}

        }

	dest[dcnt] = '\0';

	return dcnt;
}

/*
 * func : translate a utf8 buffer into gbk buffer
 *
 * args : src, srclen, the source buffer and its length
 *      : dest, dsize, dest buffer and its size
 *
 * ret  : -1, error
 *      : else, length of the result buffer
 */
/*
int py_utf8_to_s(const char* src, int srclen, char* dest, int dsize)
{
	int dcnt = 0;
	int pos  = 0;
	int len  = 0;
	unsigned short word = 0;

	for( int i = 0; i < srclen; )
        {
		word = get_utf8_word(src, srclen, &pos);
		if(word==0){
			break;
		}
		word = g_uni_s_map[word];
		len = unicode_char_to_utf8(word, dest+dcnt, dsize-dcnt);
		if(len<0){
			return -1;
		}
		dcnt += len;
        }

	dest[dcnt] = '\0';
	return 0;
}

*/
/*
 * func : translate a utf8 buffer into gbk buffer
 *
 * args : src, srclen, the source buffer and its length
 *      : dest, dsize, dest buffer and its size
 *
 * ret  : -1, error
 *      : else, length of the result buffer
 */
int py_utf8_normalize(const char* src, int srclen, char* dest, int dsize)
{
	int dcnt = 0;
	int pos  = 0;
	int len  = 0;
	unsigned short word = 0;

	for( int i = 0; i < srclen; )
        {
		word = get_utf8_word(src, srclen, &pos);
		if(word==0){
			break;
		}
		word = g_uni_normal_map[word];
		len = unicode_char_to_utf8(word, dest+dcnt, dsize-dcnt);
		if(len<0){
			return -1;
		}
		dcnt += len;
        }

	dest[dcnt] = '\0';
	return dcnt;
}
void py_unicode_normalize(wchar_t* str){
    int i;
    for(i=0;str[i];i++){
        if(str[i]>65535||str[i]<0){
            continue;
        }
        str[i]=g_uni_normal_map[str[i]];
    }
    return;
}
