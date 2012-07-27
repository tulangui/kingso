#include <zlib.h>
#include <string.h>
#include "util/compress.h"

int compress_data(char *orig_data, int orig_len, char *compr_data, int *compr_len)
{
	z_stream c_stream;
	char out_buffer[DEFLATE_CHUNK];
	int ret, have, offset = 0;
	
	c_stream.zalloc = Z_NULL;
	c_stream.zfree = Z_NULL;
	c_stream.opaque = Z_NULL;
	
	ret = deflateInit(&c_stream, Z_BEST_SPEED);
	if (ret != Z_OK)
		return -1;
	
	c_stream.avail_in = orig_len;
	c_stream.next_in = (Bytef *)orig_data;
	do {
		c_stream.avail_out = DEFLATE_CHUNK;
		c_stream.next_out = (Bytef *)out_buffer;
	
		ret = deflate(&c_stream, Z_FINISH);
		if (ret == Z_STREAM_ERROR) {
			deflateEnd(&c_stream);
			return -1;
		}
		have = DEFLATE_CHUNK - c_stream.avail_out;
		if ((*compr_len) < offset + have) {
			deflateEnd(&c_stream);
			return -1;
		}
		memcpy(compr_data + offset, out_buffer, have);
		offset += have;
	} while (c_stream.avail_out == 0);
	if (c_stream.avail_in != 0) {
		deflateEnd(&c_stream);
		return -1;
	}
	*compr_len = offset;
	deflateEnd(&c_stream);
	
	return 0;
}

int uncompress_data(char *orig_data, int orig_len, char *uncompr_data, int *uncompr_len)
{
	z_stream c_stream;
	char out_buffer[DEFLATE_CHUNK];
	int ret, have, offset = 0;
	
	c_stream.zalloc = Z_NULL;
	c_stream.zfree = Z_NULL;
	c_stream.opaque = Z_NULL;
	
	ret = inflateInit(&c_stream);
	if (ret != Z_OK)
		return -1;
	
	c_stream.avail_in = orig_len;
	c_stream.next_in = (Bytef *)orig_data;
	do {
		c_stream.avail_out = DEFLATE_CHUNK;
		c_stream.next_out = (Bytef *)out_buffer;
	
		ret = inflate(&c_stream, Z_NO_FLUSH);
		if (ret == Z_STREAM_ERROR || ret == Z_NEED_DICT || ret == Z_DATA_ERROR || ret == Z_MEM_ERROR) {
			inflateEnd(&c_stream);
			return -1;
		}
		have = DEFLATE_CHUNK - c_stream.avail_out;
		if ((*uncompr_len) < offset + have) {
			inflateEnd(&c_stream);
			return -1;
		}
		memcpy(uncompr_data + offset, out_buffer, have);
		offset += have;
	} while (c_stream.avail_out == 0);
	if (c_stream.avail_in != 0) {
		inflateEnd(&c_stream);
		return -1;
	}
	*uncompr_len = offset;
	inflateEnd(&c_stream);
	
	return 0;
}

/* 将整数n压缩写到从地址ptr开始的内存里 */
char* write_int(char *ptr, int n)
{
	while ((n&~0x7F)!=0) {
		*ptr++ = (char)((n&0x7F)|0x80);
		n >>= 7;
	}
	*ptr++ = (char)n;

	return ptr;
}

char* read_int(char *ptr, int *n)
{
    size_t shift;
    char c = *ptr ++;
    *n = c & 0x7F;
    for (shift = 7; (c&0x80)!=0; shift += 7)
    {
        c = *ptr ++;
        *n |= (c&0x7F) << shift;
    }
    return ptr;
}
