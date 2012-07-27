#include <algorithm>
#include <math.h>
#include "IndexCompression.h"

namespace index_lib {

	unpackfunc IndexCompression::unpacktable[32] = {
		IndexCompression::unpack_1,
		IndexCompression::unpack_2,
		IndexCompression::unpack_3,
		IndexCompression::unpack_4,
		IndexCompression::unpack_5,
		IndexCompression::unpack_6,
		IndexCompression::unpack_7,
		IndexCompression::unpack_8,
		IndexCompression::unpack_9,
		IndexCompression::unpack_10,
		IndexCompression::unpack_11,
		IndexCompression::unpack_12,
		IndexCompression::unpack_13,
		IndexCompression::unpack_14,
		IndexCompression::unpack_15,
		IndexCompression::unpack_16,
		IndexCompression::unpack_17,
		IndexCompression::unpack_18,
		IndexCompression::unpack_19,
		IndexCompression::unpack_20,
		IndexCompression::unpack_21,
		IndexCompression::unpack_22,
		IndexCompression::unpack_23,
		IndexCompression::unpack_24,
		IndexCompression::unpack_25,
		IndexCompression::unpack_26,
		IndexCompression::unpack_27,
		IndexCompression::unpack_28,
		IndexCompression::unpack_29,
		IndexCompression::unpack_30,
		IndexCompression::unpack_31,
		IndexCompression::unpack_32
	};

//构造函数
IndexCompression::IndexCompression()
{
	memset(m_buf, 0, (P4D_BLOCK_SIZE+1)*8);
	memset(m_unpackBuf, 0, (P4D_BLOCK_SIZE+1)*4);
	memset(m_bitsList, 0, 32);
	m_offset = 0;
	memset(m_resultList, 0, (P4D_BLOCK_SIZE+1)*sizeof(uint32_t));
	m_lastEncode = 0;
	m_lastDecode = 0;
}
//析构函数
IndexCompression::~IndexCompression()
{
}
//对外压缩入口
uint8_t* IndexCompression::compression(uint32_t *docList, int size, int exByte)
{
	int bitSize;
	m_offset = 0;
	memset(m_buf, 0, (P4D_BLOCK_SIZE+1)*8);
    getDeltaList(docList, size);	//转换成差值形式	
	bitSize = getBitSize(docList, size);	//计算压缩位数
	encode(docList, size, bitSize, exByte);	        //对差值形式的doclist进行压缩，存储于m_buf中
	return m_buf;
}

//返回压缩buf的有效长度
int IndexCompression::getCompSize()
{
	return m_offset;
}
//对外解压缩入口
uint32_t* IndexCompression::decompression(uint8_t *comBuf, int blockSize, int exByte)
{
	memset(m_resultList, 0, (P4D_BLOCK_SIZE+1)*sizeof(uint32_t));
	memset(m_unpackBuf, 0, (P4D_BLOCK_SIZE+1)*4);
	int bitSize = comBuf[0];		
	float excOffset = float(bitSize * (blockSize+1))/8.0;	//计算异常存储偏移位置
	int ceilOffset = int(ceil(excOffset));
	uint8_t *excepts = comBuf+ 1 + ceilOffset;
	uint32_t *codes = (uint32_t*)(comBuf+1);
	uint32_t unpackNum = (blockSize/32 + 1) * 32;
	
	if(blockSize < P4D_BLOCK_SIZE)
	{	
		for(int k = 0; k < ceilOffset; k++)
		{
			m_unpackBuf[k] = comBuf[k+1];
		}
		codes = (uint32_t*)(m_unpackBuf);	//压缩buf的第一个byte存储压缩位数
	}

	unpacktable[bitSize-1](m_resultList,codes,unpackNum);	//将comBuf中的压缩信息解压到result

	uint32_t next = m_resultList[0];		//第一个位置是异常跳转
	int i = 0;
	int shift = next;
	while (shift <= blockSize)	//还原异常
	{
		i += next;
		uint32_t tmp = m_resultList[i] + 1;
		m_resultList[i] = nextexcept(excepts, exByte);
		next = tmp;
		shift += next;
	}
	//comBuf = excepts;
	//m_resultList.pop_front();
    //deDelta(m_resultList+1, blockSize);	//将差值形式还原
	return m_resultList+1;
}

//将docList转换为差值形式
void IndexCompression::getDeltaList(uint32_t *docList, int size)
{
	uint32_t tmp;
	for(int i = 0; i < size; i++)
	{
		tmp = docList[i];
        docList[i] = docList[i] - m_lastEncode;
        m_lastEncode = tmp;
    }
}
//将差值形式还原
void IndexCompression::deDelta(uint32_t* deltaList, int size)
{
	for(int i = 0; i < size; i++)
	{
		deltaList[i] += m_lastDecode;
		m_lastDecode = deltaList[i];
	}
}
//计算压缩位数
int IndexCompression::getBitSize(uint32_t *deltaList, int size)
{
	int bitSize = 0;
	int offset = (int)((size > 1)?(size * BOUND_RATIO ) : 1);
	memset(m_bitsList, 0, 32);
	for(int i = 0; i < size; i++)
	{
		int bits = getBits(deltaList[i]);
		m_bitsList[bits-1] ++;
	}
	while(offset > 0)
	{
		offset -= m_bitsList[bitSize++];
	}
	return bitSize;
}

//计算一个整数的bit数
inline int IndexCompression::getBits(uint32_t value)
{
	if(value == 0)
		return 1;
	int bits;
	asm ("bsrl %1, %0;"
			:"=r"(bits)        /* output */
			:"r"(value)         /* input */
		);
	return bits+1;
}
void IndexCompression::clearEncode()
{
	m_lastEncode = 0;
}
void IndexCompression::clearDecode()
{
	m_lastDecode = 0;
}
//根据bitSize，压缩deltaList
void IndexCompression::encode(uint32_t *deltaList, int listSize, int bitSize, int exByte)
{
	uint32_t maxentry = (1u << bitSize) - 1;
	m_buf[0] = (uint8_t)bitSize;
	m_offset++;
	uint8_t *entry = m_buf + 1;
	float excOffset = float(bitSize * (listSize+1))/8.0;
	int ceilOffset = int(ceil(excOffset));
	uint8_t *excepts = entry + ceilOffset;
	m_offset += ceilOffset;
	uint32_t *encode = (uint32_t*)(entry);
	int lastexcept = -1;
	int i = 0;
	for(int j = 0; j < listSize; j++)
	{
		if (deltaList[j] > maxentry || (uint32_t)(i - lastexcept)  >= maxentry )
		{ 
			if (-1 == lastexcept)
			{
				*encode = encode[0] | (i+1);
			}
			else
			{
				writeto(i - lastexcept , encode, bitSize, lastexcept);
			}
			lastexcept = i+1;
			addexcept(deltaList[j], excepts, exByte);
		}
		else
		{
			writeto(deltaList[j], encode, bitSize, i+1);
		}
		i++;
	}
	if (lastexcept != -1)
	{
		writeto(maxentry, encode, bitSize, lastexcept);
	}
	else
	{
		*entry |= (uint8_t)maxentry;
	}
}

//向压缩存储区域写入一个data
void IndexCompression::writeto(uint32_t data, uint32_t *encode,int width, int i)
{ 
	int b = i * width;
	int e = b + width -1;
	int B = b >> 5;
	int E = e >> 5;
	int ls = b - ( B << 5 );
	encode[B] |= data << ls;
	if (E != B)
	{
		encode[E] |= data >> ( 32 - ls );
	}
}
//向压缩存储区域后端增加一个异常
void IndexCompression::addexcept(uint32_t data, uint8_t *&excepts, int bytes)
{
	uint32_t *p = (uint32_t*)excepts;
	*p = data;
	excepts += bytes;
	m_offset += bytes;
}
//取出一个异常，顺次取出
uint32_t IndexCompression::nextexcept(uint8_t *&excepts, int bytes)
{
	//uint32_t mask = (bytes < 4) ? (1u << (bytes << 3 )) - 1 : 0xFFFFFFFF;
	uint32_t *p = (uint32_t*)excepts;
	excepts += bytes;
	return *p;
}
//解压缩到output中
void IndexCompression:: 
unpack_1(uint32_t *output, uint32_t *encode, int n)
{
	int j = 0;for(int i = 32; i <= n; i+=32){
		output[j+0] |= encode[0] & 0x1;
		output[j+1] |= (encode[0] >> 1) & 0x1;
		output[j+2] |= (encode[0] >> 2) & 0x1;
		output[j+3] |= (encode[0] >> 3) & 0x1;
		output[j+4] |= (encode[0] >> 4) & 0x1;
		output[j+5] |= (encode[0] >> 5) & 0x1;
		output[j+6] |= (encode[0] >> 6) & 0x1;
		output[j+7] |= (encode[0] >> 7) & 0x1;
		output[j+8] |= (encode[0] >> 8) & 0x1;
		output[j+9] |= (encode[0] >> 9) & 0x1;
		output[j+10] |= (encode[0] >> 10) & 0x1;
		output[j+11] |= (encode[0] >> 11) & 0x1;
		output[j+12] |= (encode[0] >> 12) & 0x1;
		output[j+13] |= (encode[0] >> 13) & 0x1;
		output[j+14] |= (encode[0] >> 14) & 0x1;
		output[j+15] |= (encode[0] >> 15) & 0x1;
		output[j+16] |= (encode[0] >> 16) & 0x1;
		output[j+17] |= (encode[0] >> 17) & 0x1;
		output[j+18] |= (encode[0] >> 18) & 0x1;
		output[j+19] |= (encode[0] >> 19) & 0x1;
		output[j+20] |= (encode[0] >> 20) & 0x1;
		output[j+21] |= (encode[0] >> 21) & 0x1;
		output[j+22] |= (encode[0] >> 22) & 0x1;
		output[j+23] |= (encode[0] >> 23) & 0x1;
		output[j+24] |= (encode[0] >> 24) & 0x1;
		output[j+25] |= (encode[0] >> 25) & 0x1;
		output[j+26] |= (encode[0] >> 26) & 0x1;
		output[j+27] |= (encode[0] >> 27) & 0x1;
		output[j+28] |= (encode[0] >> 28) & 0x1;
		output[j+29] |= (encode[0] >> 29) & 0x1;
		output[j+30] |= (encode[0] >> 30) & 0x1;
		output[j+31] |= (encode[0] >> 31) & 0x1;
		encode += 1;
		j += 32;
	}
}

void IndexCompression:: 
unpack_2(uint32_t *output, uint32_t *encode, int n)
{
	int j = 0;for(int i = 32; i <= n; i+=32){
		output[j+0] |= encode[0] & 0x3;
		output[j+1] |= (encode[0] >> 2) & 0x3;
		output[j+2] |= (encode[0] >> 4) & 0x3;
		output[j+3] |= (encode[0] >> 6) & 0x3;
		output[j+4] |= (encode[0] >> 8) & 0x3;
		output[j+5] |= (encode[0] >> 10) & 0x3;
		output[j+6] |= (encode[0] >> 12) & 0x3;
		output[j+7] |= (encode[0] >> 14) & 0x3;
		output[j+8] |= (encode[0] >> 16) & 0x3;
		output[j+9] |= (encode[0] >> 18) & 0x3;
		output[j+10] |= (encode[0] >> 20) & 0x3;
		output[j+11] |= (encode[0] >> 22) & 0x3;
		output[j+12] |= (encode[0] >> 24) & 0x3;
		output[j+13] |= (encode[0] >> 26) & 0x3;
		output[j+14] |= (encode[0] >> 28) & 0x3;
		output[j+15] |= (encode[0] >> 30) & 0x3;
		output[j+16] |= encode[1] & 0x3;
		output[j+17] |= (encode[1] >> 2) & 0x3;
		output[j+18] |= (encode[1] >> 4) & 0x3;
		output[j+19] |= (encode[1] >> 6) & 0x3;
		output[j+20] |= (encode[1] >> 8) & 0x3;
		output[j+21] |= (encode[1] >> 10) & 0x3;
		output[j+22] |= (encode[1] >> 12) & 0x3;
		output[j+23] |= (encode[1] >> 14) & 0x3;
		output[j+24] |= (encode[1] >> 16) & 0x3;
		output[j+25] |= (encode[1] >> 18) & 0x3;
		output[j+26] |= (encode[1] >> 20) & 0x3;
		output[j+27] |= (encode[1] >> 22) & 0x3;
		output[j+28] |= (encode[1] >> 24) & 0x3;
		output[j+29] |= (encode[1] >> 26) & 0x3;
		output[j+30] |= (encode[1] >> 28) & 0x3;
		output[j+31] |= (encode[1] >> 30) & 0x3;
		encode += 2;
		j += 32;
	}
}

void IndexCompression:: 
unpack_3(uint32_t *output, uint32_t *encode, int n)
{
	int j = 0;for(int i = 32; i <= n; i+=32){
		output[j+0] |= encode[0] & 0x7;
		output[j+1] |= (encode[0] >> 3) & 0x7;
		output[j+2] |= (encode[0] >> 6) & 0x7;
		output[j+3] |= (encode[0] >> 9) & 0x7;
		output[j+4] |= (encode[0] >> 12) & 0x7;
		output[j+5] |= (encode[0] >> 15) & 0x7;
		output[j+6] |= (encode[0] >> 18) & 0x7;
		output[j+7] |= (encode[0] >> 21) & 0x7;
		output[j+8] |= (encode[0] >> 24) & 0x7;
		output[j+9] |= (encode[0] >> 27) & 0x7;
		output[j+10] |= ((encode[0] >> 30) | (encode[1] << 2)) & 0x7;
		output[j+11] |= (encode[1] >> 1) & 0x7;
		output[j+12] |= (encode[1] >> 4) & 0x7;
		output[j+13] |= (encode[1] >> 7) & 0x7;
		output[j+14] |= (encode[1] >> 10) & 0x7;
		output[j+15] |= (encode[1] >> 13) & 0x7;
		output[j+16] |= (encode[1] >> 16) & 0x7;
		output[j+17] |= (encode[1] >> 19) & 0x7;
		output[j+18] |= (encode[1] >> 22) & 0x7;
		output[j+19] |= (encode[1] >> 25) & 0x7;
		output[j+20] |= (encode[1] >> 28) & 0x7;
		output[j+21] |= ((encode[1] >> 31) | (encode[2] << 1)) & 0x7;
		output[j+22] |= (encode[2] >> 2) & 0x7;
		output[j+23] |= (encode[2] >> 5) & 0x7;
		output[j+24] |= (encode[2] >> 8) & 0x7;
		output[j+25] |= (encode[2] >> 11) & 0x7;
		output[j+26] |= (encode[2] >> 14) & 0x7;
		output[j+27] |= (encode[2] >> 17) & 0x7;
		output[j+28] |= (encode[2] >> 20) & 0x7;
		output[j+29] |= (encode[2] >> 23) & 0x7;
		output[j+30] |= (encode[2] >> 26) & 0x7;
		output[j+31] |= (encode[2] >> 29) & 0x7;
		encode += 3;
		j += 32;
	}
}

void IndexCompression:: 
unpack_4(uint32_t *output, uint32_t *encode, int n)
{
	int j = 0;for(int i = 32; i <= n; i+=32){
		output[j+0] |= encode[0] & 0xF;
		output[j+1] |= (encode[0] >> 4) & 0xF;
		output[j+2] |= (encode[0] >> 8) & 0xF;
		output[j+3] |= (encode[0] >> 12) & 0xF;
		output[j+4] |= (encode[0] >> 16) & 0xF;
		output[j+5] |= (encode[0] >> 20) & 0xF;
		output[j+6] |= (encode[0] >> 24) & 0xF;
		output[j+7] |= (encode[0] >> 28) & 0xF;
		output[j+8] |= encode[1] & 0xF;
		output[j+9] |= (encode[1] >> 4) & 0xF;
		output[j+10] |= (encode[1] >> 8) & 0xF;
		output[j+11] |= (encode[1] >> 12) & 0xF;
		output[j+12] |= (encode[1] >> 16) & 0xF;
		output[j+13] |= (encode[1] >> 20) & 0xF;
		output[j+14] |= (encode[1] >> 24) & 0xF;
		output[j+15] |= (encode[1] >> 28) & 0xF;
		output[j+16] |= encode[2] & 0xF;
		output[j+17] |= (encode[2] >> 4) & 0xF;
		output[j+18] |= (encode[2] >> 8) & 0xF;
		output[j+19] |= (encode[2] >> 12) & 0xF;
		output[j+20] |= (encode[2] >> 16) & 0xF;
		output[j+21] |= (encode[2] >> 20) & 0xF;
		output[j+22] |= (encode[2] >> 24) & 0xF;
		output[j+23] |= (encode[2] >> 28) & 0xF;
		output[j+24] |= encode[3] & 0xF;
		output[j+25] |= (encode[3] >> 4) & 0xF;
		output[j+26] |= (encode[3] >> 8) & 0xF;
		output[j+27] |= (encode[3] >> 12) & 0xF;
		output[j+28] |= (encode[3] >> 16) & 0xF;
		output[j+29] |= (encode[3] >> 20) & 0xF;
		output[j+30] |= (encode[3] >> 24) & 0xF;
		output[j+31] |= (encode[3] >> 28) & 0xF;
		encode += 4;
		j += 32;
	}
}

void IndexCompression:: 
unpack_5(uint32_t *output, uint32_t *encode, int n)
{
	int j = 0;for(int i = 32; i <= n; i+=32){
		output[j+0] |= encode[0] & 0x1F;
		output[j+1] |= (encode[0] >> 5) & 0x1F;
		output[j+2] |= (encode[0] >> 10) & 0x1F;
		output[j+3] |= (encode[0] >> 15) & 0x1F;
		output[j+4] |= (encode[0] >> 20) & 0x1F;
		output[j+5] |= (encode[0] >> 25) & 0x1F;
		output[j+6] |= ((encode[0] >> 30) | (encode[1] << 2)) & 0x1F;
		output[j+7] |= (encode[1] >> 3) & 0x1F;
		output[j+8] |= (encode[1] >> 8) & 0x1F;
		output[j+9] |= (encode[1] >> 13) & 0x1F;
		output[j+10] |= (encode[1] >> 18) & 0x1F;
		output[j+11] |= (encode[1] >> 23) & 0x1F;
		output[j+12] |= ((encode[1] >> 28) | (encode[2] << 4)) & 0x1F;
		output[j+13] |= (encode[2] >> 1) & 0x1F;
		output[j+14] |= (encode[2] >> 6) & 0x1F;
		output[j+15] |= (encode[2] >> 11) & 0x1F;
		output[j+16] |= (encode[2] >> 16) & 0x1F;
		output[j+17] |= (encode[2] >> 21) & 0x1F;
		output[j+18] |= (encode[2] >> 26) & 0x1F;
		output[j+19] |= ((encode[2] >> 31) | (encode[3] << 1)) & 0x1F;
		output[j+20] |= (encode[3] >> 4) & 0x1F;
		output[j+21] |= (encode[3] >> 9) & 0x1F;
		output[j+22] |= (encode[3] >> 14) & 0x1F;
		output[j+23] |= (encode[3] >> 19) & 0x1F;
		output[j+24] |= (encode[3] >> 24) & 0x1F;
		output[j+25] |= ((encode[3] >> 29) | (encode[4] << 3)) & 0x1F;
		output[j+26] |= (encode[4] >> 2) & 0x1F;
		output[j+27] |= (encode[4] >> 7) & 0x1F;
		output[j+28] |= (encode[4] >> 12) & 0x1F;
		output[j+29] |= (encode[4] >> 17) & 0x1F;
		output[j+30] |= (encode[4] >> 22) & 0x1F;
		output[j+31] |= (encode[4] >> 27) & 0x1F;
		encode += 5;
		j += 32;
	}
}

void IndexCompression:: 
unpack_6(uint32_t *output, uint32_t *encode, int n)
{
	int j = 0;for(int i = 32; i <= n; i+=32){
		output[j+0] |= encode[0] & 0x3F;
		output[j+1] |= (encode[0] >> 6) & 0x3F;
		output[j+2] |= (encode[0] >> 12) & 0x3F;
		output[j+3] |= (encode[0] >> 18) & 0x3F;
		output[j+4] |= (encode[0] >> 24) & 0x3F;
		output[j+5] |= ((encode[0] >> 30) | (encode[1] << 2)) & 0x3F;
		output[j+6] |= (encode[1] >> 4) & 0x3F;
		output[j+7] |= (encode[1] >> 10) & 0x3F;
		output[j+8] |= (encode[1] >> 16) & 0x3F;
		output[j+9] |= (encode[1] >> 22) & 0x3F;
		output[j+10] |= ((encode[1] >> 28) | (encode[2] << 4)) & 0x3F;
		output[j+11] |= (encode[2] >> 2) & 0x3F;
		output[j+12] |= (encode[2] >> 8) & 0x3F;
		output[j+13] |= (encode[2] >> 14) & 0x3F;
		output[j+14] |= (encode[2] >> 20) & 0x3F;
		output[j+15] |= (encode[2] >> 26) & 0x3F;
		output[j+16] |= encode[3] & 0x3F;
		output[j+17] |= (encode[3] >> 6) & 0x3F;
		output[j+18] |= (encode[3] >> 12) & 0x3F;
		output[j+19] |= (encode[3] >> 18) & 0x3F;
		output[j+20] |= (encode[3] >> 24) & 0x3F;
		output[j+21] |= ((encode[3] >> 30) | (encode[4] << 2)) & 0x3F;
		output[j+22] |= (encode[4] >> 4) & 0x3F;
		output[j+23] |= (encode[4] >> 10) & 0x3F;
		output[j+24] |= (encode[4] >> 16) & 0x3F;
		output[j+25] |= (encode[4] >> 22) & 0x3F;
		output[j+26] |= ((encode[4] >> 28) | (encode[5] << 4)) & 0x3F;
		output[j+27] |= (encode[5] >> 2) & 0x3F;
		output[j+28] |= (encode[5] >> 8) & 0x3F;
		output[j+29] |= (encode[5] >> 14) & 0x3F;
		output[j+30] |= (encode[5] >> 20) & 0x3F;
		output[j+31] |= (encode[5] >> 26) & 0x3F;
		encode += 6;
		j += 32;
	}
}

void IndexCompression:: 
unpack_7(uint32_t *output, uint32_t *encode, int n)
{
	int j = 0;for(int i = 32; i <= n; i+=32){
		output[j+0] |= encode[0] & 0x7F;
		output[j+1] |= (encode[0] >> 7) & 0x7F;
		output[j+2] |= (encode[0] >> 14) & 0x7F;
		output[j+3] |= (encode[0] >> 21) & 0x7F;
		output[j+4] |= ((encode[0] >> 28) | (encode[1] << 4)) & 0x7F;
		output[j+5] |= (encode[1] >> 3) & 0x7F;
		output[j+6] |= (encode[1] >> 10) & 0x7F;
		output[j+7] |= (encode[1] >> 17) & 0x7F;
		output[j+8] |= (encode[1] >> 24) & 0x7F;
		output[j+9] |= ((encode[1] >> 31) | (encode[2] << 1)) & 0x7F;
		output[j+10] |= (encode[2] >> 6) & 0x7F;
		output[j+11] |= (encode[2] >> 13) & 0x7F;
		output[j+12] |= (encode[2] >> 20) & 0x7F;
		output[j+13] |= ((encode[2] >> 27) | (encode[3] << 5)) & 0x7F;
		output[j+14] |= (encode[3] >> 2) & 0x7F;
		output[j+15] |= (encode[3] >> 9) & 0x7F;
		output[j+16] |= (encode[3] >> 16) & 0x7F;
		output[j+17] |= (encode[3] >> 23) & 0x7F;
		output[j+18] |= ((encode[3] >> 30) | (encode[4] << 2)) & 0x7F;
		output[j+19] |= (encode[4] >> 5) & 0x7F;
		output[j+20] |= (encode[4] >> 12) & 0x7F;
		output[j+21] |= (encode[4] >> 19) & 0x7F;
		output[j+22] |= ((encode[4] >> 26) | (encode[5] << 6)) & 0x7F;
		output[j+23] |= (encode[5] >> 1) & 0x7F;
		output[j+24] |= (encode[5] >> 8) & 0x7F;
		output[j+25] |= (encode[5] >> 15) & 0x7F;
		output[j+26] |= (encode[5] >> 22) & 0x7F;
		output[j+27] |= ((encode[5] >> 29) | (encode[6] << 3)) & 0x7F;
		output[j+28] |= (encode[6] >> 4) & 0x7F;
		output[j+29] |= (encode[6] >> 11) & 0x7F;
		output[j+30] |= (encode[6] >> 18) & 0x7F;
		output[j+31] |= (encode[6] >> 25) & 0x7F;
		encode += 7;
		j += 32;
	}
}

void IndexCompression:: 
unpack_8(uint32_t *output, uint32_t *encode, int n)
{
	int j = 0;for(int i = 32; i <= n; i+=32){
		output[j+0] |= encode[0] & 0xFF;
		output[j+1] |= (encode[0] >> 8) & 0xFF;
		output[j+2] |= (encode[0] >> 16) & 0xFF;
		output[j+3] |= (encode[0] >> 24) & 0xFF;
		output[j+4] |= encode[1] & 0xFF;
		output[j+5] |= (encode[1] >> 8) & 0xFF;
		output[j+6] |= (encode[1] >> 16) & 0xFF;
		output[j+7] |= (encode[1] >> 24) & 0xFF;
		output[j+8] |= encode[2] & 0xFF;
		output[j+9] |= (encode[2] >> 8) & 0xFF;
		output[j+10] |= (encode[2] >> 16) & 0xFF;
		output[j+11] |= (encode[2] >> 24) & 0xFF;
		output[j+12] |= encode[3] & 0xFF;
		output[j+13] |= (encode[3] >> 8) & 0xFF;
		output[j+14] |= (encode[3] >> 16) & 0xFF;
		output[j+15] |= (encode[3] >> 24) & 0xFF;
		output[j+16] |= encode[4] & 0xFF;
		output[j+17] |= (encode[4] >> 8) & 0xFF;
		output[j+18] |= (encode[4] >> 16) & 0xFF;
		output[j+19] |= (encode[4] >> 24) & 0xFF;
		output[j+20] |= encode[5] & 0xFF;
		output[j+21] |= (encode[5] >> 8) & 0xFF;
		output[j+22] |= (encode[5] >> 16) & 0xFF;
		output[j+23] |= (encode[5] >> 24) & 0xFF;
		output[j+24] |= encode[6] & 0xFF;
		output[j+25] |= (encode[6] >> 8) & 0xFF;
		output[j+26] |= (encode[6] >> 16) & 0xFF;
		output[j+27] |= (encode[6] >> 24) & 0xFF;
		output[j+28] |= encode[7] & 0xFF;
		output[j+29] |= (encode[7] >> 8) & 0xFF;
		output[j+30] |= (encode[7] >> 16) & 0xFF;
		output[j+31] |= (encode[7] >> 24) & 0xFF;
		encode += 8;
		j += 32;
	}
}

void IndexCompression:: 
unpack_9(uint32_t *output, uint32_t *encode, int n)
{
	int j = 0;for(int i = 32; i <= n; i+=32){
		output[j+0] |= encode[0] & 0x1FF;
		output[j+1] |= (encode[0] >> 9) & 0x1FF;
		output[j+2] |= (encode[0] >> 18) & 0x1FF;
		output[j+3] |= ((encode[0] >> 27) | (encode[1] << 5)) & 0x1FF;
		output[j+4] |= (encode[1] >> 4) & 0x1FF;
		output[j+5] |= (encode[1] >> 13) & 0x1FF;
		output[j+6] |= (encode[1] >> 22) & 0x1FF;
		output[j+7] |= ((encode[1] >> 31) | (encode[2] << 1)) & 0x1FF;
		output[j+8] |= (encode[2] >> 8) & 0x1FF;
		output[j+9] |= (encode[2] >> 17) & 0x1FF;
		output[j+10] |= ((encode[2] >> 26) | (encode[3] << 6)) & 0x1FF;
		output[j+11] |= (encode[3] >> 3) & 0x1FF;
		output[j+12] |= (encode[3] >> 12) & 0x1FF;
		output[j+13] |= (encode[3] >> 21) & 0x1FF;
		output[j+14] |= ((encode[3] >> 30) | (encode[4] << 2)) & 0x1FF;
		output[j+15] |= (encode[4] >> 7) & 0x1FF;
		output[j+16] |= (encode[4] >> 16) & 0x1FF;
		output[j+17] |= ((encode[4] >> 25) | (encode[5] << 7)) & 0x1FF;
		output[j+18] |= (encode[5] >> 2) & 0x1FF;
		output[j+19] |= (encode[5] >> 11) & 0x1FF;
		output[j+20] |= (encode[5] >> 20) & 0x1FF;
		output[j+21] |= ((encode[5] >> 29) | (encode[6] << 3)) & 0x1FF;
		output[j+22] |= (encode[6] >> 6) & 0x1FF;
		output[j+23] |= (encode[6] >> 15) & 0x1FF;
		output[j+24] |= ((encode[6] >> 24) | (encode[7] << 8)) & 0x1FF;
		output[j+25] |= (encode[7] >> 1) & 0x1FF;
		output[j+26] |= (encode[7] >> 10) & 0x1FF;
		output[j+27] |= (encode[7] >> 19) & 0x1FF;
		output[j+28] |= ((encode[7] >> 28) | (encode[8] << 4)) & 0x1FF;
		output[j+29] |= (encode[8] >> 5) & 0x1FF;
		output[j+30] |= (encode[8] >> 14) & 0x1FF;
		output[j+31] |= (encode[8] >> 23) & 0x1FF;
		encode += 9;
		j += 32;
	}
}

void IndexCompression:: 
unpack_10(uint32_t *output, uint32_t *encode, int n)
{
	int j = 0;for(int i = 32; i <= n; i+=32){
		output[j+0] |= encode[0] & 0x3FF;
		output[j+1] |= (encode[0] >> 10) & 0x3FF;
		output[j+2] |= (encode[0] >> 20) & 0x3FF;
		output[j+3] |= ((encode[0] >> 30) | (encode[1] << 2)) & 0x3FF;
		output[j+4] |= (encode[1] >> 8) & 0x3FF;
		output[j+5] |= (encode[1] >> 18) & 0x3FF;
		output[j+6] |= ((encode[1] >> 28) | (encode[2] << 4)) & 0x3FF;
		output[j+7] |= (encode[2] >> 6) & 0x3FF;
		output[j+8] |= (encode[2] >> 16) & 0x3FF;
		output[j+9] |= ((encode[2] >> 26) | (encode[3] << 6)) & 0x3FF;
		output[j+10] |= (encode[3] >> 4) & 0x3FF;
		output[j+11] |= (encode[3] >> 14) & 0x3FF;
		output[j+12] |= ((encode[3] >> 24) | (encode[4] << 8)) & 0x3FF;
		output[j+13] |= (encode[4] >> 2) & 0x3FF;
		output[j+14] |= (encode[4] >> 12) & 0x3FF;
		output[j+15] |= (encode[4] >> 22) & 0x3FF;
		output[j+16] |= encode[5] & 0x3FF;
		output[j+17] |= (encode[5] >> 10) & 0x3FF;
		output[j+18] |= (encode[5] >> 20) & 0x3FF;
		output[j+19] |= ((encode[5] >> 30) | (encode[6] << 2)) & 0x3FF;
		output[j+20] |= (encode[6] >> 8) & 0x3FF;
		output[j+21] |= (encode[6] >> 18) & 0x3FF;
		output[j+22] |= ((encode[6] >> 28) | (encode[7] << 4)) & 0x3FF;
		output[j+23] |= (encode[7] >> 6) & 0x3FF;
		output[j+24] |= (encode[7] >> 16) & 0x3FF;
		output[j+25] |= ((encode[7] >> 26) | (encode[8] << 6)) & 0x3FF;
		output[j+26] |= (encode[8] >> 4) & 0x3FF;
		output[j+27] |= (encode[8] >> 14) & 0x3FF;
		output[j+28] |= ((encode[8] >> 24) | (encode[9] << 8)) & 0x3FF;
		output[j+29] |= (encode[9] >> 2) & 0x3FF;
		output[j+30] |= (encode[9] >> 12) & 0x3FF;
		output[j+31] |= (encode[9] >> 22) & 0x3FF;
		encode += 10;
		j += 32;
	}
}

void IndexCompression:: 
unpack_11(uint32_t *output, uint32_t *encode, int n)
{
	int j = 0;for(int i = 32; i <= n; i+=32){
		output[j+0] |= encode[0] & 0x7FF;
		output[j+1] |= (encode[0] >> 11) & 0x7FF;
		output[j+2] |= ((encode[0] >> 22) | (encode[1] << 10)) & 0x7FF;
		output[j+3] |= (encode[1] >> 1) & 0x7FF;
		output[j+4] |= (encode[1] >> 12) & 0x7FF;
		output[j+5] |= ((encode[1] >> 23) | (encode[2] << 9)) & 0x7FF;
		output[j+6] |= (encode[2] >> 2) & 0x7FF;
		output[j+7] |= (encode[2] >> 13) & 0x7FF;
		output[j+8] |= ((encode[2] >> 24) | (encode[3] << 8)) & 0x7FF;
		output[j+9] |= (encode[3] >> 3) & 0x7FF;
		output[j+10] |= (encode[3] >> 14) & 0x7FF;
		output[j+11] |= ((encode[3] >> 25) | (encode[4] << 7)) & 0x7FF;
		output[j+12] |= (encode[4] >> 4) & 0x7FF;
		output[j+13] |= (encode[4] >> 15) & 0x7FF;
		output[j+14] |= ((encode[4] >> 26) | (encode[5] << 6)) & 0x7FF;
		output[j+15] |= (encode[5] >> 5) & 0x7FF;
		output[j+16] |= (encode[5] >> 16) & 0x7FF;
		output[j+17] |= ((encode[5] >> 27) | (encode[6] << 5)) & 0x7FF;
		output[j+18] |= (encode[6] >> 6) & 0x7FF;
		output[j+19] |= (encode[6] >> 17) & 0x7FF;
		output[j+20] |= ((encode[6] >> 28) | (encode[7] << 4)) & 0x7FF;
		output[j+21] |= (encode[7] >> 7) & 0x7FF;
		output[j+22] |= (encode[7] >> 18) & 0x7FF;
		output[j+23] |= ((encode[7] >> 29) | (encode[8] << 3)) & 0x7FF;
		output[j+24] |= (encode[8] >> 8) & 0x7FF;
		output[j+25] |= (encode[8] >> 19) & 0x7FF;
		output[j+26] |= ((encode[8] >> 30) | (encode[9] << 2)) & 0x7FF;
		output[j+27] |= (encode[9] >> 9) & 0x7FF;
		output[j+28] |= (encode[9] >> 20) & 0x7FF;
		output[j+29] |= ((encode[9] >> 31) | (encode[10] << 1)) & 0x7FF;
		output[j+30] |= (encode[10] >> 10) & 0x7FF;
		output[j+31] |= (encode[10] >> 21) & 0x7FF;
		encode += 11;
		j += 32;
	}
}

void IndexCompression:: 
unpack_12(uint32_t *output, uint32_t *encode, int n)
{
	int j = 0;for(int i = 32; i <= n; i+=32){
		output[j+0] |= encode[0] & 0xFFF;
		output[j+1] |= (encode[0] >> 12) & 0xFFF;
		output[j+2] |= ((encode[0] >> 24) | (encode[1] << 8)) & 0xFFF;
		output[j+3] |= (encode[1] >> 4) & 0xFFF;
		output[j+4] |= (encode[1] >> 16) & 0xFFF;
		output[j+5] |= ((encode[1] >> 28) | (encode[2] << 4)) & 0xFFF;
		output[j+6] |= (encode[2] >> 8) & 0xFFF;
		output[j+7] |= (encode[2] >> 20) & 0xFFF;
		output[j+8] |= encode[3] & 0xFFF;
		output[j+9] |= (encode[3] >> 12) & 0xFFF;
		output[j+10] |= ((encode[3] >> 24) | (encode[4] << 8)) & 0xFFF;
		output[j+11] |= (encode[4] >> 4) & 0xFFF;
		output[j+12] |= (encode[4] >> 16) & 0xFFF;
		output[j+13] |= ((encode[4] >> 28) | (encode[5] << 4)) & 0xFFF;
		output[j+14] |= (encode[5] >> 8) & 0xFFF;
		output[j+15] |= (encode[5] >> 20) & 0xFFF;
		output[j+16] |= encode[6] & 0xFFF;
		output[j+17] |= (encode[6] >> 12) & 0xFFF;
		output[j+18] |= ((encode[6] >> 24) | (encode[7] << 8)) & 0xFFF;
		output[j+19] |= (encode[7] >> 4) & 0xFFF;
		output[j+20] |= (encode[7] >> 16) & 0xFFF;
		output[j+21] |= ((encode[7] >> 28) | (encode[8] << 4)) & 0xFFF;
		output[j+22] |= (encode[8] >> 8) & 0xFFF;
		output[j+23] |= (encode[8] >> 20) & 0xFFF;
		output[j+24] |= encode[9] & 0xFFF;
		output[j+25] |= (encode[9] >> 12) & 0xFFF;
		output[j+26] |= ((encode[9] >> 24) | (encode[10] << 8)) & 0xFFF;
		output[j+27] |= (encode[10] >> 4) & 0xFFF;
		output[j+28] |= (encode[10] >> 16) & 0xFFF;
		output[j+29] |= ((encode[10] >> 28) | (encode[11] << 4)) & 0xFFF;
		output[j+30] |= (encode[11] >> 8) & 0xFFF;
		output[j+31] |= (encode[11] >> 20) & 0xFFF;
		encode += 12;
		j += 32;
	}
}

void IndexCompression:: 
unpack_13(uint32_t *output, uint32_t *encode, int n)
{
	int j = 0;for(int i = 32; i <= n; i+=32){
		output[j+0] |= encode[0] & 0x1FFF;
		output[j+1] |= (encode[0] >> 13) & 0x1FFF;
		output[j+2] |= ((encode[0] >> 26) | (encode[1] << 6)) & 0x1FFF;
		output[j+3] |= (encode[1] >> 7) & 0x1FFF;
		output[j+4] |= ((encode[1] >> 20) | (encode[2] << 12)) & 0x1FFF;
		output[j+5] |= (encode[2] >> 1) & 0x1FFF;
		output[j+6] |= (encode[2] >> 14) & 0x1FFF;
		output[j+7] |= ((encode[2] >> 27) | (encode[3] << 5)) & 0x1FFF;
		output[j+8] |= (encode[3] >> 8) & 0x1FFF;
		output[j+9] |= ((encode[3] >> 21) | (encode[4] << 11)) & 0x1FFF;
		output[j+10] |= (encode[4] >> 2) & 0x1FFF;
		output[j+11] |= (encode[4] >> 15) & 0x1FFF;
		output[j+12] |= ((encode[4] >> 28) | (encode[5] << 4)) & 0x1FFF;
		output[j+13] |= (encode[5] >> 9) & 0x1FFF;
		output[j+14] |= ((encode[5] >> 22) | (encode[6] << 10)) & 0x1FFF;
		output[j+15] |= (encode[6] >> 3) & 0x1FFF;
		output[j+16] |= (encode[6] >> 16) & 0x1FFF;
		output[j+17] |= ((encode[6] >> 29) | (encode[7] << 3)) & 0x1FFF;
		output[j+18] |= (encode[7] >> 10) & 0x1FFF;
		output[j+19] |= ((encode[7] >> 23) | (encode[8] << 9)) & 0x1FFF;
		output[j+20] |= (encode[8] >> 4) & 0x1FFF;
		output[j+21] |= (encode[8] >> 17) & 0x1FFF;
		output[j+22] |= ((encode[8] >> 30) | (encode[9] << 2)) & 0x1FFF;
		output[j+23] |= (encode[9] >> 11) & 0x1FFF;
		output[j+24] |= ((encode[9] >> 24) | (encode[10] << 8)) & 0x1FFF;
		output[j+25] |= (encode[10] >> 5) & 0x1FFF;
		output[j+26] |= (encode[10] >> 18) & 0x1FFF;
		output[j+27] |= ((encode[10] >> 31) | (encode[11] << 1)) & 0x1FFF;
		output[j+28] |= (encode[11] >> 12) & 0x1FFF;
		output[j+29] |= ((encode[11] >> 25) | (encode[12] << 7)) & 0x1FFF;
		output[j+30] |= (encode[12] >> 6) & 0x1FFF;
		output[j+31] |= (encode[12] >> 19) & 0x1FFF;
		encode += 13;
		j += 32;
	}
}

void IndexCompression:: 
unpack_14(uint32_t *output, uint32_t *encode, int n)
{
	int j = 0;for(int i = 32; i <= n; i+=32){
		output[j+0] |= encode[0] & 0x3FFF;
		output[j+1] |= (encode[0] >> 14) & 0x3FFF;
		output[j+2] |= ((encode[0] >> 28) | (encode[1] << 4)) & 0x3FFF;
		output[j+3] |= (encode[1] >> 10) & 0x3FFF;
		output[j+4] |= ((encode[1] >> 24) | (encode[2] << 8)) & 0x3FFF;
		output[j+5] |= (encode[2] >> 6) & 0x3FFF;
		output[j+6] |= ((encode[2] >> 20) | (encode[3] << 12)) & 0x3FFF;
		output[j+7] |= (encode[3] >> 2) & 0x3FFF;
		output[j+8] |= (encode[3] >> 16) & 0x3FFF;
		output[j+9] |= ((encode[3] >> 30) | (encode[4] << 2)) & 0x3FFF;
		output[j+10] |= (encode[4] >> 12) & 0x3FFF;
		output[j+11] |= ((encode[4] >> 26) | (encode[5] << 6)) & 0x3FFF;
		output[j+12] |= (encode[5] >> 8) & 0x3FFF;
		output[j+13] |= ((encode[5] >> 22) | (encode[6] << 10)) & 0x3FFF;
		output[j+14] |= (encode[6] >> 4) & 0x3FFF;
		output[j+15] |= (encode[6] >> 18) & 0x3FFF;
		output[j+16] |= encode[7] & 0x3FFF;
		output[j+17] |= (encode[7] >> 14) & 0x3FFF;
		output[j+18] |= ((encode[7] >> 28) | (encode[8] << 4)) & 0x3FFF;
		output[j+19] |= (encode[8] >> 10) & 0x3FFF;
		output[j+20] |= ((encode[8] >> 24) | (encode[9] << 8)) & 0x3FFF;
		output[j+21] |= (encode[9] >> 6) & 0x3FFF;
		output[j+22] |= ((encode[9] >> 20) | (encode[10] << 12)) & 0x3FFF;
		output[j+23] |= (encode[10] >> 2) & 0x3FFF;
		output[j+24] |= (encode[10] >> 16) & 0x3FFF;
		output[j+25] |= ((encode[10] >> 30) | (encode[11] << 2)) & 0x3FFF;
		output[j+26] |= (encode[11] >> 12) & 0x3FFF;
		output[j+27] |= ((encode[11] >> 26) | (encode[12] << 6)) & 0x3FFF;
		output[j+28] |= (encode[12] >> 8) & 0x3FFF;
		output[j+29] |= ((encode[12] >> 22) | (encode[13] << 10)) & 0x3FFF;
		output[j+30] |= (encode[13] >> 4) & 0x3FFF;
		output[j+31] |= (encode[13] >> 18) & 0x3FFF;
		encode += 14;
		j += 32;
	}
}

void IndexCompression:: 
unpack_15(uint32_t *output, uint32_t *encode, int n)
{
	int j = 0;for(int i = 32; i <= n; i+=32){
		output[j+0] |= encode[0] & 0x7FFF;
		output[j+1] |= (encode[0] >> 15) & 0x7FFF;
		output[j+2] |= ((encode[0] >> 30) | (encode[1] << 2)) & 0x7FFF;
		output[j+3] |= (encode[1] >> 13) & 0x7FFF;
		output[j+4] |= ((encode[1] >> 28) | (encode[2] << 4)) & 0x7FFF;
		output[j+5] |= (encode[2] >> 11) & 0x7FFF;
		output[j+6] |= ((encode[2] >> 26) | (encode[3] << 6)) & 0x7FFF;
		output[j+7] |= (encode[3] >> 9) & 0x7FFF;
		output[j+8] |= ((encode[3] >> 24) | (encode[4] << 8)) & 0x7FFF;
		output[j+9] |= (encode[4] >> 7) & 0x7FFF;
		output[j+10] |= ((encode[4] >> 22) | (encode[5] << 10)) & 0x7FFF;
		output[j+11] |= (encode[5] >> 5) & 0x7FFF;
		output[j+12] |= ((encode[5] >> 20) | (encode[6] << 12)) & 0x7FFF;
		output[j+13] |= (encode[6] >> 3) & 0x7FFF;
		output[j+14] |= ((encode[6] >> 18) | (encode[7] << 14)) & 0x7FFF;
		output[j+15] |= (encode[7] >> 1) & 0x7FFF;
		output[j+16] |= (encode[7] >> 16) & 0x7FFF;
		output[j+17] |= ((encode[7] >> 31) | (encode[8] << 1)) & 0x7FFF;
		output[j+18] |= (encode[8] >> 14) & 0x7FFF;
		output[j+19] |= ((encode[8] >> 29) | (encode[9] << 3)) & 0x7FFF;
		output[j+20] |= (encode[9] >> 12) & 0x7FFF;
		output[j+21] |= ((encode[9] >> 27) | (encode[10] << 5)) & 0x7FFF;
		output[j+22] |= (encode[10] >> 10) & 0x7FFF;
		output[j+23] |= ((encode[10] >> 25) | (encode[11] << 7)) & 0x7FFF;
		output[j+24] |= (encode[11] >> 8) & 0x7FFF;
		output[j+25] |= ((encode[11] >> 23) | (encode[12] << 9)) & 0x7FFF;
		output[j+26] |= (encode[12] >> 6) & 0x7FFF;
		output[j+27] |= ((encode[12] >> 21) | (encode[13] << 11)) & 0x7FFF;
		output[j+28] |= (encode[13] >> 4) & 0x7FFF;
		output[j+29] |= ((encode[13] >> 19) | (encode[14] << 13)) & 0x7FFF;
		output[j+30] |= (encode[14] >> 2) & 0x7FFF;
		output[j+31] |= (encode[14] >> 17) & 0x7FFF;
		encode += 15;
		j += 32;
	}
}

void IndexCompression:: 
unpack_16(uint32_t *output, uint32_t *encode, int n)
{
	int j = 0;for(int i = 32; i <= n; i+=32){
		output[j+0] |= encode[0] & 0xFFFF;
		output[j+1] |= (encode[0] >> 16) & 0xFFFF;
		output[j+2] |= encode[1] & 0xFFFF;
		output[j+3] |= (encode[1] >> 16) & 0xFFFF;
		output[j+4] |= encode[2] & 0xFFFF;
		output[j+5] |= (encode[2] >> 16) & 0xFFFF;
		output[j+6] |= encode[3] & 0xFFFF;
		output[j+7] |= (encode[3] >> 16) & 0xFFFF;
		output[j+8] |= encode[4] & 0xFFFF;
		output[j+9] |= (encode[4] >> 16) & 0xFFFF;
		output[j+10] |= encode[5] & 0xFFFF;
		output[j+11] |= (encode[5] >> 16) & 0xFFFF;
		output[j+12] |= encode[6] & 0xFFFF;
		output[j+13] |= (encode[6] >> 16) & 0xFFFF;
		output[j+14] |= encode[7] & 0xFFFF;
		output[j+15] |= (encode[7] >> 16) & 0xFFFF;
		output[j+16] |= encode[8] & 0xFFFF;
		output[j+17] |= (encode[8] >> 16) & 0xFFFF;
		output[j+18] |= encode[9] & 0xFFFF;
		output[j+19] |= (encode[9] >> 16) & 0xFFFF;
		output[j+20] |= encode[10] & 0xFFFF;
		output[j+21] |= (encode[10] >> 16) & 0xFFFF;
		output[j+22] |= encode[11] & 0xFFFF;
		output[j+23] |= (encode[11] >> 16) & 0xFFFF;
		output[j+24] |= encode[12] & 0xFFFF;
		output[j+25] |= (encode[12] >> 16) & 0xFFFF;
		output[j+26] |= encode[13] & 0xFFFF;
		output[j+27] |= (encode[13] >> 16) & 0xFFFF;
		output[j+28] |= encode[14] & 0xFFFF;
		output[j+29] |= (encode[14] >> 16) & 0xFFFF;
		output[j+30] |= encode[15] & 0xFFFF;
		output[j+31] |= (encode[15] >> 16) & 0xFFFF;
		encode += 16;
		j += 32;
	}
}

void IndexCompression:: 
unpack_17(uint32_t *output, uint32_t *encode, int n)
{
	int j = 0;for(int i = 32; i <= n; i+=32){
		output[j+0] |= encode[0] & 0x1FFFF;
		output[j+1] |= ((encode[0] >> 17) | (encode[1] << 15)) & 0x1FFFF;
		output[j+2] |= (encode[1] >> 2) & 0x1FFFF;
		output[j+3] |= ((encode[1] >> 19) | (encode[2] << 13)) & 0x1FFFF;
		output[j+4] |= (encode[2] >> 4) & 0x1FFFF;
		output[j+5] |= ((encode[2] >> 21) | (encode[3] << 11)) & 0x1FFFF;
		output[j+6] |= (encode[3] >> 6) & 0x1FFFF;
		output[j+7] |= ((encode[3] >> 23) | (encode[4] << 9)) & 0x1FFFF;
		output[j+8] |= (encode[4] >> 8) & 0x1FFFF;
		output[j+9] |= ((encode[4] >> 25) | (encode[5] << 7)) & 0x1FFFF;
		output[j+10] |= (encode[5] >> 10) & 0x1FFFF;
		output[j+11] |= ((encode[5] >> 27) | (encode[6] << 5)) & 0x1FFFF;
		output[j+12] |= (encode[6] >> 12) & 0x1FFFF;
		output[j+13] |= ((encode[6] >> 29) | (encode[7] << 3)) & 0x1FFFF;
		output[j+14] |= (encode[7] >> 14) & 0x1FFFF;
		output[j+15] |= ((encode[7] >> 31) | (encode[8] << 1)) & 0x1FFFF;
		output[j+16] |= ((encode[8] >> 16) | (encode[9] << 16)) & 0x1FFFF;
		output[j+17] |= (encode[9] >> 1) & 0x1FFFF;
		output[j+18] |= ((encode[9] >> 18) | (encode[10] << 14)) & 0x1FFFF;
		output[j+19] |= (encode[10] >> 3) & 0x1FFFF;
		output[j+20] |= ((encode[10] >> 20) | (encode[11] << 12)) & 0x1FFFF;
		output[j+21] |= (encode[11] >> 5) & 0x1FFFF;
		output[j+22] |= ((encode[11] >> 22) | (encode[12] << 10)) & 0x1FFFF;
		output[j+23] |= (encode[12] >> 7) & 0x1FFFF;
		output[j+24] |= ((encode[12] >> 24) | (encode[13] << 8)) & 0x1FFFF;
		output[j+25] |= (encode[13] >> 9) & 0x1FFFF;
		output[j+26] |= ((encode[13] >> 26) | (encode[14] << 6)) & 0x1FFFF;
		output[j+27] |= (encode[14] >> 11) & 0x1FFFF;
		output[j+28] |= ((encode[14] >> 28) | (encode[15] << 4)) & 0x1FFFF;
		output[j+29] |= (encode[15] >> 13) & 0x1FFFF;
		output[j+30] |= ((encode[15] >> 30) | (encode[16] << 2)) & 0x1FFFF;
		output[j+31] |= (encode[16] >> 15) & 0x1FFFF;
		encode += 17;
		j += 32;
	}
}

void IndexCompression:: 
unpack_18(uint32_t *output, uint32_t *encode, int n)
{
	int j = 0;for(int i = 32; i <= n; i+=32){
		output[j+0] |= encode[0] & 0x3FFFF;
		output[j+1] |= ((encode[0] >> 18) | (encode[1] << 14)) & 0x3FFFF;
		output[j+2] |= (encode[1] >> 4) & 0x3FFFF;
		output[j+3] |= ((encode[1] >> 22) | (encode[2] << 10)) & 0x3FFFF;
		output[j+4] |= (encode[2] >> 8) & 0x3FFFF;
		output[j+5] |= ((encode[2] >> 26) | (encode[3] << 6)) & 0x3FFFF;
		output[j+6] |= (encode[3] >> 12) & 0x3FFFF;
		output[j+7] |= ((encode[3] >> 30) | (encode[4] << 2)) & 0x3FFFF;
		output[j+8] |= ((encode[4] >> 16) | (encode[5] << 16)) & 0x3FFFF;
		output[j+9] |= (encode[5] >> 2) & 0x3FFFF;
		output[j+10] |= ((encode[5] >> 20) | (encode[6] << 12)) & 0x3FFFF;
		output[j+11] |= (encode[6] >> 6) & 0x3FFFF;
		output[j+12] |= ((encode[6] >> 24) | (encode[7] << 8)) & 0x3FFFF;
		output[j+13] |= (encode[7] >> 10) & 0x3FFFF;
		output[j+14] |= ((encode[7] >> 28) | (encode[8] << 4)) & 0x3FFFF;
		output[j+15] |= (encode[8] >> 14) & 0x3FFFF;
		output[j+16] |= encode[9] & 0x3FFFF;
		output[j+17] |= ((encode[9] >> 18) | (encode[10] << 14)) & 0x3FFFF;
		output[j+18] |= (encode[10] >> 4) & 0x3FFFF;
		output[j+19] |= ((encode[10] >> 22) | (encode[11] << 10)) & 0x3FFFF;
		output[j+20] |= (encode[11] >> 8) & 0x3FFFF;
		output[j+21] |= ((encode[11] >> 26) | (encode[12] << 6)) & 0x3FFFF;
		output[j+22] |= (encode[12] >> 12) & 0x3FFFF;
		output[j+23] |= ((encode[12] >> 30) | (encode[13] << 2)) & 0x3FFFF;
		output[j+24] |= ((encode[13] >> 16) | (encode[14] << 16)) & 0x3FFFF;
		output[j+25] |= (encode[14] >> 2) & 0x3FFFF;
		output[j+26] |= ((encode[14] >> 20) | (encode[15] << 12)) & 0x3FFFF;
		output[j+27] |= (encode[15] >> 6) & 0x3FFFF;
		output[j+28] |= ((encode[15] >> 24) | (encode[16] << 8)) & 0x3FFFF;
		output[j+29] |= (encode[16] >> 10) & 0x3FFFF;
		output[j+30] |= ((encode[16] >> 28) | (encode[17] << 4)) & 0x3FFFF;
		output[j+31] |= (encode[17] >> 14) & 0x3FFFF;
		encode += 18;
		j += 32;
	}
}

void IndexCompression:: 
unpack_19(uint32_t *output, uint32_t *encode, int n)
{
	int j = 0;for(int i = 32; i <= n; i+=32){
		output[j+0] |= encode[0] & 0x7FFFF;
		output[j+1] |= ((encode[0] >> 19) | (encode[1] << 13)) & 0x7FFFF;
		output[j+2] |= (encode[1] >> 6) & 0x7FFFF;
		output[j+3] |= ((encode[1] >> 25) | (encode[2] << 7)) & 0x7FFFF;
		output[j+4] |= (encode[2] >> 12) & 0x7FFFF;
		output[j+5] |= ((encode[2] >> 31) | (encode[3] << 1)) & 0x7FFFF;
		output[j+6] |= ((encode[3] >> 18) | (encode[4] << 14)) & 0x7FFFF;
		output[j+7] |= (encode[4] >> 5) & 0x7FFFF;
		output[j+8] |= ((encode[4] >> 24) | (encode[5] << 8)) & 0x7FFFF;
		output[j+9] |= (encode[5] >> 11) & 0x7FFFF;
		output[j+10] |= ((encode[5] >> 30) | (encode[6] << 2)) & 0x7FFFF;
		output[j+11] |= ((encode[6] >> 17) | (encode[7] << 15)) & 0x7FFFF;
		output[j+12] |= (encode[7] >> 4) & 0x7FFFF;
		output[j+13] |= ((encode[7] >> 23) | (encode[8] << 9)) & 0x7FFFF;
		output[j+14] |= (encode[8] >> 10) & 0x7FFFF;
		output[j+15] |= ((encode[8] >> 29) | (encode[9] << 3)) & 0x7FFFF;
		output[j+16] |= ((encode[9] >> 16) | (encode[10] << 16)) & 0x7FFFF;
		output[j+17] |= (encode[10] >> 3) & 0x7FFFF;
		output[j+18] |= ((encode[10] >> 22) | (encode[11] << 10)) & 0x7FFFF;
		output[j+19] |= (encode[11] >> 9) & 0x7FFFF;
		output[j+20] |= ((encode[11] >> 28) | (encode[12] << 4)) & 0x7FFFF;
		output[j+21] |= ((encode[12] >> 15) | (encode[13] << 17)) & 0x7FFFF;
		output[j+22] |= (encode[13] >> 2) & 0x7FFFF;
		output[j+23] |= ((encode[13] >> 21) | (encode[14] << 11)) & 0x7FFFF;
		output[j+24] |= (encode[14] >> 8) & 0x7FFFF;
		output[j+25] |= ((encode[14] >> 27) | (encode[15] << 5)) & 0x7FFFF;
		output[j+26] |= ((encode[15] >> 14) | (encode[16] << 18)) & 0x7FFFF;
		output[j+27] |= (encode[16] >> 1) & 0x7FFFF;
		output[j+28] |= ((encode[16] >> 20) | (encode[17] << 12)) & 0x7FFFF;
		output[j+29] |= (encode[17] >> 7) & 0x7FFFF;
		output[j+30] |= ((encode[17] >> 26) | (encode[18] << 6)) & 0x7FFFF;
		output[j+31] |= (encode[18] >> 13) & 0x7FFFF;
		encode += 19;
		j += 32;
	}
}

void IndexCompression:: 
unpack_20(uint32_t *output, uint32_t *encode, int n)
{
	int j = 0;for(int i = 32; i <= n; i+=32){
		output[j+0] |= encode[0] & 0xFFFFF;
		output[j+1] |= ((encode[0] >> 20) | (encode[1] << 12)) & 0xFFFFF;
		output[j+2] |= (encode[1] >> 8) & 0xFFFFF;
		output[j+3] |= ((encode[1] >> 28) | (encode[2] << 4)) & 0xFFFFF;
		output[j+4] |= ((encode[2] >> 16) | (encode[3] << 16)) & 0xFFFFF;
		output[j+5] |= (encode[3] >> 4) & 0xFFFFF;
		output[j+6] |= ((encode[3] >> 24) | (encode[4] << 8)) & 0xFFFFF;
		output[j+7] |= (encode[4] >> 12) & 0xFFFFF;
		output[j+8] |= encode[5] & 0xFFFFF;
		output[j+9] |= ((encode[5] >> 20) | (encode[6] << 12)) & 0xFFFFF;
		output[j+10] |= (encode[6] >> 8) & 0xFFFFF;
		output[j+11] |= ((encode[6] >> 28) | (encode[7] << 4)) & 0xFFFFF;
		output[j+12] |= ((encode[7] >> 16) | (encode[8] << 16)) & 0xFFFFF;
		output[j+13] |= (encode[8] >> 4) & 0xFFFFF;
		output[j+14] |= ((encode[8] >> 24) | (encode[9] << 8)) & 0xFFFFF;
		output[j+15] |= (encode[9] >> 12) & 0xFFFFF;
		output[j+16] |= encode[10] & 0xFFFFF;
		output[j+17] |= ((encode[10] >> 20) | (encode[11] << 12)) & 0xFFFFF;
		output[j+18] |= (encode[11] >> 8) & 0xFFFFF;
		output[j+19] |= ((encode[11] >> 28) | (encode[12] << 4)) & 0xFFFFF;
		output[j+20] |= ((encode[12] >> 16) | (encode[13] << 16)) & 0xFFFFF;
		output[j+21] |= (encode[13] >> 4) & 0xFFFFF;
		output[j+22] |= ((encode[13] >> 24) | (encode[14] << 8)) & 0xFFFFF;
		output[j+23] |= (encode[14] >> 12) & 0xFFFFF;
		output[j+24] |= encode[15] & 0xFFFFF;
		output[j+25] |= ((encode[15] >> 20) | (encode[16] << 12)) & 0xFFFFF;
		output[j+26] |= (encode[16] >> 8) & 0xFFFFF;
		output[j+27] |= ((encode[16] >> 28) | (encode[17] << 4)) & 0xFFFFF;
		output[j+28] |= ((encode[17] >> 16) | (encode[18] << 16)) & 0xFFFFF;
		output[j+29] |= (encode[18] >> 4) & 0xFFFFF;
		output[j+30] |= ((encode[18] >> 24) | (encode[19] << 8)) & 0xFFFFF;
		output[j+31] |= (encode[19] >> 12) & 0xFFFFF;
		encode += 20;
		j += 32;
	}
}

void IndexCompression:: 
unpack_21(uint32_t *output, uint32_t *encode, int n)
{
	int j = 0;for(int i = 32; i <= n; i+=32){
		output[j+0] |= encode[0] & 0x1FFFFF;
		output[j+1] |= ((encode[0] >> 21) | (encode[1] << 11)) & 0x1FFFFF;
		output[j+2] |= (encode[1] >> 10) & 0x1FFFFF;
		output[j+3] |= ((encode[1] >> 31) | (encode[2] << 1)) & 0x1FFFFF;
		output[j+4] |= ((encode[2] >> 20) | (encode[3] << 12)) & 0x1FFFFF;
		output[j+5] |= (encode[3] >> 9) & 0x1FFFFF;
		output[j+6] |= ((encode[3] >> 30) | (encode[4] << 2)) & 0x1FFFFF;
		output[j+7] |= ((encode[4] >> 19) | (encode[5] << 13)) & 0x1FFFFF;
		output[j+8] |= (encode[5] >> 8) & 0x1FFFFF;
		output[j+9] |= ((encode[5] >> 29) | (encode[6] << 3)) & 0x1FFFFF;
		output[j+10] |= ((encode[6] >> 18) | (encode[7] << 14)) & 0x1FFFFF;
		output[j+11] |= (encode[7] >> 7) & 0x1FFFFF;
		output[j+12] |= ((encode[7] >> 28) | (encode[8] << 4)) & 0x1FFFFF;
		output[j+13] |= ((encode[8] >> 17) | (encode[9] << 15)) & 0x1FFFFF;
		output[j+14] |= (encode[9] >> 6) & 0x1FFFFF;
		output[j+15] |= ((encode[9] >> 27) | (encode[10] << 5)) & 0x1FFFFF;
		output[j+16] |= ((encode[10] >> 16) | (encode[11] << 16)) & 0x1FFFFF;
		output[j+17] |= (encode[11] >> 5) & 0x1FFFFF;
		output[j+18] |= ((encode[11] >> 26) | (encode[12] << 6)) & 0x1FFFFF;
		output[j+19] |= ((encode[12] >> 15) | (encode[13] << 17)) & 0x1FFFFF;
		output[j+20] |= (encode[13] >> 4) & 0x1FFFFF;
		output[j+21] |= ((encode[13] >> 25) | (encode[14] << 7)) & 0x1FFFFF;
		output[j+22] |= ((encode[14] >> 14) | (encode[15] << 18)) & 0x1FFFFF;
		output[j+23] |= (encode[15] >> 3) & 0x1FFFFF;
		output[j+24] |= ((encode[15] >> 24) | (encode[16] << 8)) & 0x1FFFFF;
		output[j+25] |= ((encode[16] >> 13) | (encode[17] << 19)) & 0x1FFFFF;
		output[j+26] |= (encode[17] >> 2) & 0x1FFFFF;
		output[j+27] |= ((encode[17] >> 23) | (encode[18] << 9)) & 0x1FFFFF;
		output[j+28] |= ((encode[18] >> 12) | (encode[19] << 20)) & 0x1FFFFF;
		output[j+29] |= (encode[19] >> 1) & 0x1FFFFF;
		output[j+30] |= ((encode[19] >> 22) | (encode[20] << 10)) & 0x1FFFFF;
		output[j+31] |= (encode[20] >> 11) & 0x1FFFFF;
		encode += 21;
		j += 32;
	}
}

void IndexCompression:: 
unpack_22(uint32_t *output, uint32_t *encode, int n)
{
	int j = 0;for(int i = 32; i <= n; i+=32){
		output[j+0] |= encode[0] & 0x3FFFFF;
		output[j+1] |= ((encode[0] >> 22) | (encode[1] << 10)) & 0x3FFFFF;
		output[j+2] |= ((encode[1] >> 12) | (encode[2] << 20)) & 0x3FFFFF;
		output[j+3] |= (encode[2] >> 2) & 0x3FFFFF;
		output[j+4] |= ((encode[2] >> 24) | (encode[3] << 8)) & 0x3FFFFF;
		output[j+5] |= ((encode[3] >> 14) | (encode[4] << 18)) & 0x3FFFFF;
		output[j+6] |= (encode[4] >> 4) & 0x3FFFFF;
		output[j+7] |= ((encode[4] >> 26) | (encode[5] << 6)) & 0x3FFFFF;
		output[j+8] |= ((encode[5] >> 16) | (encode[6] << 16)) & 0x3FFFFF;
		output[j+9] |= (encode[6] >> 6) & 0x3FFFFF;
		output[j+10] |= ((encode[6] >> 28) | (encode[7] << 4)) & 0x3FFFFF;
		output[j+11] |= ((encode[7] >> 18) | (encode[8] << 14)) & 0x3FFFFF;
		output[j+12] |= (encode[8] >> 8) & 0x3FFFFF;
		output[j+13] |= ((encode[8] >> 30) | (encode[9] << 2)) & 0x3FFFFF;
		output[j+14] |= ((encode[9] >> 20) | (encode[10] << 12)) & 0x3FFFFF;
		output[j+15] |= (encode[10] >> 10) & 0x3FFFFF;
		output[j+16] |= encode[11] & 0x3FFFFF;
		output[j+17] |= ((encode[11] >> 22) | (encode[12] << 10)) & 0x3FFFFF;
		output[j+18] |= ((encode[12] >> 12) | (encode[13] << 20)) & 0x3FFFFF;
		output[j+19] |= (encode[13] >> 2) & 0x3FFFFF;
		output[j+20] |= ((encode[13] >> 24) | (encode[14] << 8)) & 0x3FFFFF;
		output[j+21] |= ((encode[14] >> 14) | (encode[15] << 18)) & 0x3FFFFF;
		output[j+22] |= (encode[15] >> 4) & 0x3FFFFF;
		output[j+23] |= ((encode[15] >> 26) | (encode[16] << 6)) & 0x3FFFFF;
		output[j+24] |= ((encode[16] >> 16) | (encode[17] << 16)) & 0x3FFFFF;
		output[j+25] |= (encode[17] >> 6) & 0x3FFFFF;
		output[j+26] |= ((encode[17] >> 28) | (encode[18] << 4)) & 0x3FFFFF;
		output[j+27] |= ((encode[18] >> 18) | (encode[19] << 14)) & 0x3FFFFF;
		output[j+28] |= (encode[19] >> 8) & 0x3FFFFF;
		output[j+29] |= ((encode[19] >> 30) | (encode[20] << 2)) & 0x3FFFFF;
		output[j+30] |= ((encode[20] >> 20) | (encode[21] << 12)) & 0x3FFFFF;
		output[j+31] |= (encode[21] >> 10) & 0x3FFFFF;
		encode += 22;
		j += 32;
	}
}

void IndexCompression:: 
unpack_23(uint32_t *output, uint32_t *encode, int n)
{
	int j = 0;for(int i = 32; i <= n; i+=32){
		output[j+0] |= encode[0] & 0x7FFFFF;
		output[j+1] |= ((encode[0] >> 23) | (encode[1] << 9)) & 0x7FFFFF;
		output[j+2] |= ((encode[1] >> 14) | (encode[2] << 18)) & 0x7FFFFF;
		output[j+3] |= (encode[2] >> 5) & 0x7FFFFF;
		output[j+4] |= ((encode[2] >> 28) | (encode[3] << 4)) & 0x7FFFFF;
		output[j+5] |= ((encode[3] >> 19) | (encode[4] << 13)) & 0x7FFFFF;
		output[j+6] |= ((encode[4] >> 10) | (encode[5] << 22)) & 0x7FFFFF;
		output[j+7] |= (encode[5] >> 1) & 0x7FFFFF;
		output[j+8] |= ((encode[5] >> 24) | (encode[6] << 8)) & 0x7FFFFF;
		output[j+9] |= ((encode[6] >> 15) | (encode[7] << 17)) & 0x7FFFFF;
		output[j+10] |= (encode[7] >> 6) & 0x7FFFFF;
		output[j+11] |= ((encode[7] >> 29) | (encode[8] << 3)) & 0x7FFFFF;
		output[j+12] |= ((encode[8] >> 20) | (encode[9] << 12)) & 0x7FFFFF;
		output[j+13] |= ((encode[9] >> 11) | (encode[10] << 21)) & 0x7FFFFF;
		output[j+14] |= (encode[10] >> 2) & 0x7FFFFF;
		output[j+15] |= ((encode[10] >> 25) | (encode[11] << 7)) & 0x7FFFFF;
		output[j+16] |= ((encode[11] >> 16) | (encode[12] << 16)) & 0x7FFFFF;
		output[j+17] |= (encode[12] >> 7) & 0x7FFFFF;
		output[j+18] |= ((encode[12] >> 30) | (encode[13] << 2)) & 0x7FFFFF;
		output[j+19] |= ((encode[13] >> 21) | (encode[14] << 11)) & 0x7FFFFF;
		output[j+20] |= ((encode[14] >> 12) | (encode[15] << 20)) & 0x7FFFFF;
		output[j+21] |= (encode[15] >> 3) & 0x7FFFFF;
		output[j+22] |= ((encode[15] >> 26) | (encode[16] << 6)) & 0x7FFFFF;
		output[j+23] |= ((encode[16] >> 17) | (encode[17] << 15)) & 0x7FFFFF;
		output[j+24] |= (encode[17] >> 8) & 0x7FFFFF;
		output[j+25] |= ((encode[17] >> 31) | (encode[18] << 1)) & 0x7FFFFF;
		output[j+26] |= ((encode[18] >> 22) | (encode[19] << 10)) & 0x7FFFFF;
		output[j+27] |= ((encode[19] >> 13) | (encode[20] << 19)) & 0x7FFFFF;
		output[j+28] |= (encode[20] >> 4) & 0x7FFFFF;
		output[j+29] |= ((encode[20] >> 27) | (encode[21] << 5)) & 0x7FFFFF;
		output[j+30] |= ((encode[21] >> 18) | (encode[22] << 14)) & 0x7FFFFF;
		output[j+31] |= (encode[22] >> 9) & 0x7FFFFF;
		encode += 23;
		j += 32;
	}
}

void IndexCompression:: 
unpack_24(uint32_t *output, uint32_t *encode, int n)
{
	int j = 0;for(int i = 32; i <= n; i+=32){
		output[j+0] |= encode[0] & 0xFFFFFF;
		output[j+1] |= ((encode[0] >> 24) | (encode[1] << 8)) & 0xFFFFFF;
		output[j+2] |= ((encode[1] >> 16) | (encode[2] << 16)) & 0xFFFFFF;
		output[j+3] |= (encode[2] >> 8) & 0xFFFFFF;
		output[j+4] |= encode[3] & 0xFFFFFF;
		output[j+5] |= ((encode[3] >> 24) | (encode[4] << 8)) & 0xFFFFFF;
		output[j+6] |= ((encode[4] >> 16) | (encode[5] << 16)) & 0xFFFFFF;
		output[j+7] |= (encode[5] >> 8) & 0xFFFFFF;
		output[j+8] |= encode[6] & 0xFFFFFF;
		output[j+9] |= ((encode[6] >> 24) | (encode[7] << 8)) & 0xFFFFFF;
		output[j+10] |= ((encode[7] >> 16) | (encode[8] << 16)) & 0xFFFFFF;
		output[j+11] |= (encode[8] >> 8) & 0xFFFFFF;
		output[j+12] |= encode[9] & 0xFFFFFF;
		output[j+13] |= ((encode[9] >> 24) | (encode[10] << 8)) & 0xFFFFFF;
		output[j+14] |= ((encode[10] >> 16) | (encode[11] << 16)) & 0xFFFFFF;
		output[j+15] |= (encode[11] >> 8) & 0xFFFFFF;
		output[j+16] |= encode[12] & 0xFFFFFF;
		output[j+17] |= ((encode[12] >> 24) | (encode[13] << 8)) & 0xFFFFFF;
		output[j+18] |= ((encode[13] >> 16) | (encode[14] << 16)) & 0xFFFFFF;
		output[j+19] |= (encode[14] >> 8) & 0xFFFFFF;
		output[j+20] |= encode[15] & 0xFFFFFF;
		output[j+21] |= ((encode[15] >> 24) | (encode[16] << 8)) & 0xFFFFFF;
		output[j+22] |= ((encode[16] >> 16) | (encode[17] << 16)) & 0xFFFFFF;
		output[j+23] |= (encode[17] >> 8) & 0xFFFFFF;
		output[j+24] |= encode[18] & 0xFFFFFF;
		output[j+25] |= ((encode[18] >> 24) | (encode[19] << 8)) & 0xFFFFFF;
		output[j+26] |= ((encode[19] >> 16) | (encode[20] << 16)) & 0xFFFFFF;
		output[j+27] |= (encode[20] >> 8) & 0xFFFFFF;
		output[j+28] |= encode[21] & 0xFFFFFF;
		output[j+29] |= ((encode[21] >> 24) | (encode[22] << 8)) & 0xFFFFFF;
		output[j+30] |= ((encode[22] >> 16) | (encode[23] << 16)) & 0xFFFFFF;
		output[j+31] |= (encode[23] >> 8) & 0xFFFFFF;
		encode += 24;
		j += 32;
	}
}

void IndexCompression:: 
unpack_25(uint32_t *output, uint32_t *encode, int n)
{
	int j = 0;for(int i = 32; i <= n; i+=32){
		output[j+0] |= encode[0] & 0x1FFFFFF;
		output[j+1] |= ((encode[0] >> 25) | (encode[1] << 7)) & 0x1FFFFFF;
		output[j+2] |= ((encode[1] >> 18) | (encode[2] << 14)) & 0x1FFFFFF;
		output[j+3] |= ((encode[2] >> 11) | (encode[3] << 21)) & 0x1FFFFFF;
		output[j+4] |= (encode[3] >> 4) & 0x1FFFFFF;
		output[j+5] |= ((encode[3] >> 29) | (encode[4] << 3)) & 0x1FFFFFF;
		output[j+6] |= ((encode[4] >> 22) | (encode[5] << 10)) & 0x1FFFFFF;
		output[j+7] |= ((encode[5] >> 15) | (encode[6] << 17)) & 0x1FFFFFF;
		output[j+8] |= ((encode[6] >> 8) | (encode[7] << 24)) & 0x1FFFFFF;
		output[j+9] |= (encode[7] >> 1) & 0x1FFFFFF;
		output[j+10] |= ((encode[7] >> 26) | (encode[8] << 6)) & 0x1FFFFFF;
		output[j+11] |= ((encode[8] >> 19) | (encode[9] << 13)) & 0x1FFFFFF;
		output[j+12] |= ((encode[9] >> 12) | (encode[10] << 20)) & 0x1FFFFFF;
		output[j+13] |= (encode[10] >> 5) & 0x1FFFFFF;
		output[j+14] |= ((encode[10] >> 30) | (encode[11] << 2)) & 0x1FFFFFF;
		output[j+15] |= ((encode[11] >> 23) | (encode[12] << 9)) & 0x1FFFFFF;
		output[j+16] |= ((encode[12] >> 16) | (encode[13] << 16)) & 0x1FFFFFF;
		output[j+17] |= ((encode[13] >> 9) | (encode[14] << 23)) & 0x1FFFFFF;
		output[j+18] |= (encode[14] >> 2) & 0x1FFFFFF;
		output[j+19] |= ((encode[14] >> 27) | (encode[15] << 5)) & 0x1FFFFFF;
		output[j+20] |= ((encode[15] >> 20) | (encode[16] << 12)) & 0x1FFFFFF;
		output[j+21] |= ((encode[16] >> 13) | (encode[17] << 19)) & 0x1FFFFFF;
		output[j+22] |= (encode[17] >> 6) & 0x1FFFFFF;
		output[j+23] |= ((encode[17] >> 31) | (encode[18] << 1)) & 0x1FFFFFF;
		output[j+24] |= ((encode[18] >> 24) | (encode[19] << 8)) & 0x1FFFFFF;
		output[j+25] |= ((encode[19] >> 17) | (encode[20] << 15)) & 0x1FFFFFF;
		output[j+26] |= ((encode[20] >> 10) | (encode[21] << 22)) & 0x1FFFFFF;
		output[j+27] |= (encode[21] >> 3) & 0x1FFFFFF;
		output[j+28] |= ((encode[21] >> 28) | (encode[22] << 4)) & 0x1FFFFFF;
		output[j+29] |= ((encode[22] >> 21) | (encode[23] << 11)) & 0x1FFFFFF;
		output[j+30] |= ((encode[23] >> 14) | (encode[24] << 18)) & 0x1FFFFFF;
		output[j+31] |= (encode[24] >> 7) & 0x1FFFFFF;
		encode += 25;
		j += 32;
	}
}

void IndexCompression:: 
unpack_26(uint32_t *output, uint32_t *encode, int n)
{
	int j = 0;for(int i = 32; i <= n; i+=32){
		output[j+0] |= encode[0] & 0x3FFFFFF;
		output[j+1] |= ((encode[0] >> 26) | (encode[1] << 6)) & 0x3FFFFFF;
		output[j+2] |= ((encode[1] >> 20) | (encode[2] << 12)) & 0x3FFFFFF;
		output[j+3] |= ((encode[2] >> 14) | (encode[3] << 18)) & 0x3FFFFFF;
		output[j+4] |= ((encode[3] >> 8) | (encode[4] << 24)) & 0x3FFFFFF;
		output[j+5] |= (encode[4] >> 2) & 0x3FFFFFF;
		output[j+6] |= ((encode[4] >> 28) | (encode[5] << 4)) & 0x3FFFFFF;
		output[j+7] |= ((encode[5] >> 22) | (encode[6] << 10)) & 0x3FFFFFF;
		output[j+8] |= ((encode[6] >> 16) | (encode[7] << 16)) & 0x3FFFFFF;
		output[j+9] |= ((encode[7] >> 10) | (encode[8] << 22)) & 0x3FFFFFF;
		output[j+10] |= (encode[8] >> 4) & 0x3FFFFFF;
		output[j+11] |= ((encode[8] >> 30) | (encode[9] << 2)) & 0x3FFFFFF;
		output[j+12] |= ((encode[9] >> 24) | (encode[10] << 8)) & 0x3FFFFFF;
		output[j+13] |= ((encode[10] >> 18) | (encode[11] << 14)) & 0x3FFFFFF;
		output[j+14] |= ((encode[11] >> 12) | (encode[12] << 20)) & 0x3FFFFFF;
		output[j+15] |= (encode[12] >> 6) & 0x3FFFFFF;
		output[j+16] |= encode[13] & 0x3FFFFFF;
		output[j+17] |= ((encode[13] >> 26) | (encode[14] << 6)) & 0x3FFFFFF;
		output[j+18] |= ((encode[14] >> 20) | (encode[15] << 12)) & 0x3FFFFFF;
		output[j+19] |= ((encode[15] >> 14) | (encode[16] << 18)) & 0x3FFFFFF;
		output[j+20] |= ((encode[16] >> 8) | (encode[17] << 24)) & 0x3FFFFFF;
		output[j+21] |= (encode[17] >> 2) & 0x3FFFFFF;
		output[j+22] |= ((encode[17] >> 28) | (encode[18] << 4)) & 0x3FFFFFF;
		output[j+23] |= ((encode[18] >> 22) | (encode[19] << 10)) & 0x3FFFFFF;
		output[j+24] |= ((encode[19] >> 16) | (encode[20] << 16)) & 0x3FFFFFF;
		output[j+25] |= ((encode[20] >> 10) | (encode[21] << 22)) & 0x3FFFFFF;
		output[j+26] |= (encode[21] >> 4) & 0x3FFFFFF;
		output[j+27] |= ((encode[21] >> 30) | (encode[22] << 2)) & 0x3FFFFFF;
		output[j+28] |= ((encode[22] >> 24) | (encode[23] << 8)) & 0x3FFFFFF;
		output[j+29] |= ((encode[23] >> 18) | (encode[24] << 14)) & 0x3FFFFFF;
		output[j+30] |= ((encode[24] >> 12) | (encode[25] << 20)) & 0x3FFFFFF;
		output[j+31] |= (encode[25] >> 6) & 0x3FFFFFF;
		encode += 26;
		j += 32;
	}
}

void IndexCompression:: 
unpack_27(uint32_t *output, uint32_t *encode, int n)
{
	int j = 0;for(int i = 32; i <= n; i+=32){
		output[j+0] |= encode[0] & 0x7FFFFFF;
		output[j+1] |= ((encode[0] >> 27) | (encode[1] << 5)) & 0x7FFFFFF;
		output[j+2] |= ((encode[1] >> 22) | (encode[2] << 10)) & 0x7FFFFFF;
		output[j+3] |= ((encode[2] >> 17) | (encode[3] << 15)) & 0x7FFFFFF;
		output[j+4] |= ((encode[3] >> 12) | (encode[4] << 20)) & 0x7FFFFFF;
		output[j+5] |= ((encode[4] >> 7) | (encode[5] << 25)) & 0x7FFFFFF;
		output[j+6] |= (encode[5] >> 2) & 0x7FFFFFF;
		output[j+7] |= ((encode[5] >> 29) | (encode[6] << 3)) & 0x7FFFFFF;
		output[j+8] |= ((encode[6] >> 24) | (encode[7] << 8)) & 0x7FFFFFF;
		output[j+9] |= ((encode[7] >> 19) | (encode[8] << 13)) & 0x7FFFFFF;
		output[j+10] |= ((encode[8] >> 14) | (encode[9] << 18)) & 0x7FFFFFF;
		output[j+11] |= ((encode[9] >> 9) | (encode[10] << 23)) & 0x7FFFFFF;
		output[j+12] |= (encode[10] >> 4) & 0x7FFFFFF;
		output[j+13] |= ((encode[10] >> 31) | (encode[11] << 1)) & 0x7FFFFFF;
		output[j+14] |= ((encode[11] >> 26) | (encode[12] << 6)) & 0x7FFFFFF;
		output[j+15] |= ((encode[12] >> 21) | (encode[13] << 11)) & 0x7FFFFFF;
		output[j+16] |= ((encode[13] >> 16) | (encode[14] << 16)) & 0x7FFFFFF;
		output[j+17] |= ((encode[14] >> 11) | (encode[15] << 21)) & 0x7FFFFFF;
		output[j+18] |= ((encode[15] >> 6) | (encode[16] << 26)) & 0x7FFFFFF;
		output[j+19] |= (encode[16] >> 1) & 0x7FFFFFF;
		output[j+20] |= ((encode[16] >> 28) | (encode[17] << 4)) & 0x7FFFFFF;
		output[j+21] |= ((encode[17] >> 23) | (encode[18] << 9)) & 0x7FFFFFF;
		output[j+22] |= ((encode[18] >> 18) | (encode[19] << 14)) & 0x7FFFFFF;
		output[j+23] |= ((encode[19] >> 13) | (encode[20] << 19)) & 0x7FFFFFF;
		output[j+24] |= ((encode[20] >> 8) | (encode[21] << 24)) & 0x7FFFFFF;
		output[j+25] |= (encode[21] >> 3) & 0x7FFFFFF;
		output[j+26] |= ((encode[21] >> 30) | (encode[22] << 2)) & 0x7FFFFFF;
		output[j+27] |= ((encode[22] >> 25) | (encode[23] << 7)) & 0x7FFFFFF;
		output[j+28] |= ((encode[23] >> 20) | (encode[24] << 12)) & 0x7FFFFFF;
		output[j+29] |= ((encode[24] >> 15) | (encode[25] << 17)) & 0x7FFFFFF;
		output[j+30] |= ((encode[25] >> 10) | (encode[26] << 22)) & 0x7FFFFFF;
		output[j+31] |= (encode[26] >> 5) & 0x7FFFFFF;
		encode += 27;
		j += 32;
	}
}

void IndexCompression:: 
unpack_28(uint32_t *output, uint32_t *encode, int n)
{
	int j = 0;for(int i = 32; i <= n; i+=32){
		output[j+0] |= encode[0] & 0xFFFFFFF;
		output[j+1] |= ((encode[0] >> 28) | (encode[1] << 4)) & 0xFFFFFFF;
		output[j+2] |= ((encode[1] >> 24) | (encode[2] << 8)) & 0xFFFFFFF;
		output[j+3] |= ((encode[2] >> 20) | (encode[3] << 12)) & 0xFFFFFFF;
		output[j+4] |= ((encode[3] >> 16) | (encode[4] << 16)) & 0xFFFFFFF;
		output[j+5] |= ((encode[4] >> 12) | (encode[5] << 20)) & 0xFFFFFFF;
		output[j+6] |= ((encode[5] >> 8) | (encode[6] << 24)) & 0xFFFFFFF;
		output[j+7] |= (encode[6] >> 4) & 0xFFFFFFF;
		output[j+8] |= encode[7] & 0xFFFFFFF;
		output[j+9] |= ((encode[7] >> 28) | (encode[8] << 4)) & 0xFFFFFFF;
		output[j+10] |= ((encode[8] >> 24) | (encode[9] << 8)) & 0xFFFFFFF;
		output[j+11] |= ((encode[9] >> 20) | (encode[10] << 12)) & 0xFFFFFFF;
		output[j+12] |= ((encode[10] >> 16) | (encode[11] << 16)) & 0xFFFFFFF;
		output[j+13] |= ((encode[11] >> 12) | (encode[12] << 20)) & 0xFFFFFFF;
		output[j+14] |= ((encode[12] >> 8) | (encode[13] << 24)) & 0xFFFFFFF;
		output[j+15] |= (encode[13] >> 4) & 0xFFFFFFF;
		output[j+16] |= encode[14] & 0xFFFFFFF;
		output[j+17] |= ((encode[14] >> 28) | (encode[15] << 4)) & 0xFFFFFFF;
		output[j+18] |= ((encode[15] >> 24) | (encode[16] << 8)) & 0xFFFFFFF;
		output[j+19] |= ((encode[16] >> 20) | (encode[17] << 12)) & 0xFFFFFFF;
		output[j+20] |= ((encode[17] >> 16) | (encode[18] << 16)) & 0xFFFFFFF;
		output[j+21] |= ((encode[18] >> 12) | (encode[19] << 20)) & 0xFFFFFFF;
		output[j+22] |= ((encode[19] >> 8) | (encode[20] << 24)) & 0xFFFFFFF;
		output[j+23] |= (encode[20] >> 4) & 0xFFFFFFF;
		output[j+24] |= encode[21] & 0xFFFFFFF;
		output[j+25] |= ((encode[21] >> 28) | (encode[22] << 4)) & 0xFFFFFFF;
		output[j+26] |= ((encode[22] >> 24) | (encode[23] << 8)) & 0xFFFFFFF;
		output[j+27] |= ((encode[23] >> 20) | (encode[24] << 12)) & 0xFFFFFFF;
		output[j+28] |= ((encode[24] >> 16) | (encode[25] << 16)) & 0xFFFFFFF;
		output[j+29] |= ((encode[25] >> 12) | (encode[26] << 20)) & 0xFFFFFFF;
		output[j+30] |= ((encode[26] >> 8) | (encode[27] << 24)) & 0xFFFFFFF;
		output[j+31] |= (encode[27] >> 4) & 0xFFFFFFF;
		encode += 28;
		j += 32;
	}
}

void IndexCompression:: 
unpack_29(uint32_t *output, uint32_t *encode, int n)
{
	int j = 0;for(int i = 32; i <= n; i+=32){
		output[j+0] |= encode[0] & 0x1FFFFFFF;
		output[j+1] |= ((encode[0] >> 29) | (encode[1] << 3)) & 0x1FFFFFFF;
		output[j+2] |= ((encode[1] >> 26) | (encode[2] << 6)) & 0x1FFFFFFF;
		output[j+3] |= ((encode[2] >> 23) | (encode[3] << 9)) & 0x1FFFFFFF;
		output[j+4] |= ((encode[3] >> 20) | (encode[4] << 12)) & 0x1FFFFFFF;
		output[j+5] |= ((encode[4] >> 17) | (encode[5] << 15)) & 0x1FFFFFFF;
		output[j+6] |= ((encode[5] >> 14) | (encode[6] << 18)) & 0x1FFFFFFF;
		output[j+7] |= ((encode[6] >> 11) | (encode[7] << 21)) & 0x1FFFFFFF;
		output[j+8] |= ((encode[7] >> 8) | (encode[8] << 24)) & 0x1FFFFFFF;
		output[j+9] |= ((encode[8] >> 5) | (encode[9] << 27)) & 0x1FFFFFFF;
		output[j+10] |= (encode[9] >> 2) & 0x1FFFFFFF;
		output[j+11] |= ((encode[9] >> 31) | (encode[10] << 1)) & 0x1FFFFFFF;
		output[j+12] |= ((encode[10] >> 28) | (encode[11] << 4)) & 0x1FFFFFFF;
		output[j+13] |= ((encode[11] >> 25) | (encode[12] << 7)) & 0x1FFFFFFF;
		output[j+14] |= ((encode[12] >> 22) | (encode[13] << 10)) & 0x1FFFFFFF;
		output[j+15] |= ((encode[13] >> 19) | (encode[14] << 13)) & 0x1FFFFFFF;
		output[j+16] |= ((encode[14] >> 16) | (encode[15] << 16)) & 0x1FFFFFFF;
		output[j+17] |= ((encode[15] >> 13) | (encode[16] << 19)) & 0x1FFFFFFF;
		output[j+18] |= ((encode[16] >> 10) | (encode[17] << 22)) & 0x1FFFFFFF;
		output[j+19] |= ((encode[17] >> 7) | (encode[18] << 25)) & 0x1FFFFFFF;
		output[j+20] |= ((encode[18] >> 4) | (encode[19] << 28)) & 0x1FFFFFFF;
		output[j+21] |= (encode[19] >> 1) & 0x1FFFFFFF;
		output[j+22] |= ((encode[19] >> 30) | (encode[20] << 2)) & 0x1FFFFFFF;
		output[j+23] |= ((encode[20] >> 27) | (encode[21] << 5)) & 0x1FFFFFFF;
		output[j+24] |= ((encode[21] >> 24) | (encode[22] << 8)) & 0x1FFFFFFF;
		output[j+25] |= ((encode[22] >> 21) | (encode[23] << 11)) & 0x1FFFFFFF;
		output[j+26] |= ((encode[23] >> 18) | (encode[24] << 14)) & 0x1FFFFFFF;
		output[j+27] |= ((encode[24] >> 15) | (encode[25] << 17)) & 0x1FFFFFFF;
		output[j+28] |= ((encode[25] >> 12) | (encode[26] << 20)) & 0x1FFFFFFF;
		output[j+29] |= ((encode[26] >> 9) | (encode[27] << 23)) & 0x1FFFFFFF;
		output[j+30] |= ((encode[27] >> 6) | (encode[28] << 26)) & 0x1FFFFFFF;
		output[j+31] |= (encode[28] >> 3) & 0x1FFFFFFF;
		encode += 29;
		j += 32;
	}
}

void IndexCompression:: 
unpack_30(uint32_t *output, uint32_t *encode, int n)
{
	int j = 0;for(int i = 32; i <= n; i+=32){
		output[j+0] |= encode[0] & 0x3FFFFFFF;
		output[j+1] |= ((encode[0] >> 30) | (encode[1] << 2)) & 0x3FFFFFFF;
		output[j+2] |= ((encode[1] >> 28) | (encode[2] << 4)) & 0x3FFFFFFF;
		output[j+3] |= ((encode[2] >> 26) | (encode[3] << 6)) & 0x3FFFFFFF;
		output[j+4] |= ((encode[3] >> 24) | (encode[4] << 8)) & 0x3FFFFFFF;
		output[j+5] |= ((encode[4] >> 22) | (encode[5] << 10)) & 0x3FFFFFFF;
		output[j+6] |= ((encode[5] >> 20) | (encode[6] << 12)) & 0x3FFFFFFF;
		output[j+7] |= ((encode[6] >> 18) | (encode[7] << 14)) & 0x3FFFFFFF;
		output[j+8] |= ((encode[7] >> 16) | (encode[8] << 16)) & 0x3FFFFFFF;
		output[j+9] |= ((encode[8] >> 14) | (encode[9] << 18)) & 0x3FFFFFFF;
		output[j+10] |= ((encode[9] >> 12) | (encode[10] << 20)) & 0x3FFFFFFF;
		output[j+11] |= ((encode[10] >> 10) | (encode[11] << 22)) & 0x3FFFFFFF;
		output[j+12] |= ((encode[11] >> 8) | (encode[12] << 24)) & 0x3FFFFFFF;
		output[j+13] |= ((encode[12] >> 6) | (encode[13] << 26)) & 0x3FFFFFFF;
		output[j+14] |= ((encode[13] >> 4) | (encode[14] << 28)) & 0x3FFFFFFF;
		output[j+15] |= (encode[14] >> 2) & 0x3FFFFFFF;
		output[j+16] |= encode[15] & 0x3FFFFFFF;
		output[j+17] |= ((encode[15] >> 30) | (encode[16] << 2)) & 0x3FFFFFFF;
		output[j+18] |= ((encode[16] >> 28) | (encode[17] << 4)) & 0x3FFFFFFF;
		output[j+19] |= ((encode[17] >> 26) | (encode[18] << 6)) & 0x3FFFFFFF;
		output[j+20] |= ((encode[18] >> 24) | (encode[19] << 8)) & 0x3FFFFFFF;
		output[j+21] |= ((encode[19] >> 22) | (encode[20] << 10)) & 0x3FFFFFFF;
		output[j+22] |= ((encode[20] >> 20) | (encode[21] << 12)) & 0x3FFFFFFF;
		output[j+23] |= ((encode[21] >> 18) | (encode[22] << 14)) & 0x3FFFFFFF;
		output[j+24] |= ((encode[22] >> 16) | (encode[23] << 16)) & 0x3FFFFFFF;
		output[j+25] |= ((encode[23] >> 14) | (encode[24] << 18)) & 0x3FFFFFFF;
		output[j+26] |= ((encode[24] >> 12) | (encode[25] << 20)) & 0x3FFFFFFF;
		output[j+27] |= ((encode[25] >> 10) | (encode[26] << 22)) & 0x3FFFFFFF;
		output[j+28] |= ((encode[26] >> 8) | (encode[27] << 24)) & 0x3FFFFFFF;
		output[j+29] |= ((encode[27] >> 6) | (encode[28] << 26)) & 0x3FFFFFFF;
		output[j+30] |= ((encode[28] >> 4) | (encode[29] << 28)) & 0x3FFFFFFF;
		output[j+31] |= (encode[29] >> 2) & 0x3FFFFFFF;
		encode += 30;
		j += 32;
	}
}

void IndexCompression:: 
unpack_31(uint32_t *output, uint32_t *encode, int n)
{
	int j = 0;for(int i = 32; i <= n; i+=32){
		output[j+0] |= encode[0] & 0x7FFFFFFF;
		output[j+1] |= ((encode[0] >> 31) | (encode[1] << 1)) & 0x7FFFFFFF;
		output[j+2] |= ((encode[1] >> 30) | (encode[2] << 2)) & 0x7FFFFFFF;
		output[j+3] |= ((encode[2] >> 29) | (encode[3] << 3)) & 0x7FFFFFFF;
		output[j+4] |= ((encode[3] >> 28) | (encode[4] << 4)) & 0x7FFFFFFF;
		output[j+5] |= ((encode[4] >> 27) | (encode[5] << 5)) & 0x7FFFFFFF;
		output[j+6] |= ((encode[5] >> 26) | (encode[6] << 6)) & 0x7FFFFFFF;
		output[j+7] |= ((encode[6] >> 25) | (encode[7] << 7)) & 0x7FFFFFFF;
		output[j+8] |= ((encode[7] >> 24) | (encode[8] << 8)) & 0x7FFFFFFF;
		output[j+9] |= ((encode[8] >> 23) | (encode[9] << 9)) & 0x7FFFFFFF;
		output[j+10] |= ((encode[9] >> 22) | (encode[10] << 10)) & 0x7FFFFFFF;
		output[j+11] |= ((encode[10] >> 21) | (encode[11] << 11)) & 0x7FFFFFFF;
		output[j+12] |= ((encode[11] >> 20) | (encode[12] << 12)) & 0x7FFFFFFF;
		output[j+13] |= ((encode[12] >> 19) | (encode[13] << 13)) & 0x7FFFFFFF;
		output[j+14] |= ((encode[13] >> 18) | (encode[14] << 14)) & 0x7FFFFFFF;
		output[j+15] |= ((encode[14] >> 17) | (encode[15] << 15)) & 0x7FFFFFFF;
		output[j+16] |= ((encode[15] >> 16) | (encode[16] << 16)) & 0x7FFFFFFF;
		output[j+17] |= ((encode[16] >> 15) | (encode[17] << 17)) & 0x7FFFFFFF;
		output[j+18] |= ((encode[17] >> 14) | (encode[18] << 18)) & 0x7FFFFFFF;
		output[j+19] |= ((encode[18] >> 13) | (encode[19] << 19)) & 0x7FFFFFFF;
		output[j+20] |= ((encode[19] >> 12) | (encode[20] << 20)) & 0x7FFFFFFF;
		output[j+21] |= ((encode[20] >> 11) | (encode[21] << 21)) & 0x7FFFFFFF;
		output[j+22] |= ((encode[21] >> 10) | (encode[22] << 22)) & 0x7FFFFFFF;
		output[j+23] |= ((encode[22] >> 9) | (encode[23] << 23)) & 0x7FFFFFFF;
		output[j+24] |= ((encode[23] >> 8) | (encode[24] << 24)) & 0x7FFFFFFF;
		output[j+25] |= ((encode[24] >> 7) | (encode[25] << 25)) & 0x7FFFFFFF;
		output[j+26] |= ((encode[25] >> 6) | (encode[26] << 26)) & 0x7FFFFFFF;
		output[j+27] |= ((encode[26] >> 5) | (encode[27] << 27)) & 0x7FFFFFFF;
		output[j+28] |= ((encode[27] >> 4) | (encode[28] << 28)) & 0x7FFFFFFF;
		output[j+29] |= ((encode[28] >> 3) | (encode[29] << 29)) & 0x7FFFFFFF;
		output[j+30] |= ((encode[29] >> 2) | (encode[30] << 30)) & 0x7FFFFFFF;
		output[j+31] |= (encode[30] >> 1) & 0x7FFFFFFF;
		encode += 31;
		j += 32;
	}
}

void IndexCompression:: 
unpack_32(uint32_t *output, uint32_t *encode, int n)
{
	int j = 0;for(int i = 32; i <= n; i+=32){
		output[j+0] |= encode[0] & 0xFFFFFFFF;
		output[j+1] |= encode[1] & 0xFFFFFFFF;
		output[j+2] |= encode[2] & 0xFFFFFFFF;
		output[j+3] |= encode[3] & 0xFFFFFFFF;
		output[j+4] |= encode[4] & 0xFFFFFFFF;
		output[j+5] |= encode[5] & 0xFFFFFFFF;
		output[j+6] |= encode[6] & 0xFFFFFFFF;
		output[j+7] |= encode[7] & 0xFFFFFFFF;
		output[j+8] |= encode[8] & 0xFFFFFFFF;
		output[j+9] |= encode[9] & 0xFFFFFFFF;
		output[j+10] |= encode[10] & 0xFFFFFFFF;
		output[j+11] |= encode[11] & 0xFFFFFFFF;
		output[j+12] |= encode[12] & 0xFFFFFFFF;
		output[j+13] |= encode[13] & 0xFFFFFFFF;
		output[j+14] |= encode[14] & 0xFFFFFFFF;
		output[j+15] |= encode[15] & 0xFFFFFFFF;
		output[j+16] |= encode[16] & 0xFFFFFFFF;
		output[j+17] |= encode[17] & 0xFFFFFFFF;
		output[j+18] |= encode[18] & 0xFFFFFFFF;
		output[j+19] |= encode[19] & 0xFFFFFFFF;
		output[j+20] |= encode[20] & 0xFFFFFFFF;
		output[j+21] |= encode[21] & 0xFFFFFFFF;
		output[j+22] |= encode[22] & 0xFFFFFFFF;
		output[j+23] |= encode[23] & 0xFFFFFFFF;
		output[j+24] |= encode[24] & 0xFFFFFFFF;
		output[j+25] |= encode[25] & 0xFFFFFFFF;
		output[j+26] |= encode[26] & 0xFFFFFFFF;
		output[j+27] |= encode[27] & 0xFFFFFFFF;
		output[j+28] |= encode[28] & 0xFFFFFFFF;
		output[j+29] |= encode[29] & 0xFFFFFFFF;
		output[j+30] |= encode[30] & 0xFFFFFFFF;
		output[j+31] |= encode[31] & 0xFFFFFFFF;
		encode += 32;
		j += 32;
	}
}

}
