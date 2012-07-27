#ifndef INDEX_COMPRESSION_H
#define INDEX_COMPRESSION_H

#define P4D_BLOCK_SIZE 127
#define BOUND_RATIO 0.9
#define EXCEPTION_BYTE 4

namespace index_lib {

typedef void (*unpackfunc)(uint32_t *, uint32_t *, int);

class IndexCompression
{
	public:
		IndexCompression();
		~IndexCompression();
		uint8_t* compression(uint32_t *docList, int size, int exByte = EXCEPTION_BYTE);	 //压缩入口
		int getCompSize();	//换回压缩后的字节数
		uint32_t* decompression(uint8_t *comBuf, int size, int exByte = EXCEPTION_BYTE);	//解压缩入口
		void clearEncode();
		void clearDecode();
				
	private:
		void getDeltaList(uint32_t *docList, int size);	//将原始doclist转换成差值形式
		void deDelta(uint32_t *docList, int size);	//将差值形式的doclist还原
		int getBitSize(uint32_t *deltaList, int size);	//计算压缩bit长度
		void encode(uint32_t *docList, int size, int bitSize, int exByte);	//压缩差值形式的doclist，保存在m_buf中
		void writeto(uint32_t data, uint32_t *encode,int width, int i);	//向buf中写一个docid
		void addexcept(uint32_t data, uint8_t *&excepts, int bytes);	//向buf中加入一个异常
		uint32_t nextexcept(uint8_t *&excepts, int bytes);	//取出一个异常
		inline int getBits(uint32_t);
		static void unpack_1(uint32_t *output, uint32_t *encode, int bitSize);
		static void unpack_2(uint32_t *output, uint32_t *encode, int bitSize);
		static void unpack_3(uint32_t *output, uint32_t *encode, int bitSize);
		static void unpack_4(uint32_t *output, uint32_t *encode, int bitSize);
		static void unpack_5(uint32_t *output, uint32_t *encode, int bitSize);
		static void unpack_6(uint32_t *output, uint32_t *encode, int bitSize);
		static void unpack_7(uint32_t *output, uint32_t *encode, int bitSize);
		static void unpack_8(uint32_t *output, uint32_t *encode, int bitSize);
		static void unpack_9(uint32_t *output, uint32_t *encode, int bitSize);
		static void unpack_10(uint32_t *output, uint32_t *encode, int bitSize);
		static void unpack_11(uint32_t *output, uint32_t *encode, int bitSize);
		static void unpack_12(uint32_t *output, uint32_t *encode, int bitSize);
		static void unpack_13(uint32_t *output, uint32_t *encode, int bitSize);
		static void unpack_14(uint32_t *output, uint32_t *encode, int bitSize);
		static void unpack_15(uint32_t *output, uint32_t *encode, int bitSize);
		static void unpack_16(uint32_t *output, uint32_t *encode, int bitSize);
		static void unpack_17(uint32_t *output, uint32_t *encode, int bitSize);
		static void unpack_18(uint32_t *output, uint32_t *encode, int bitSize);
		static void unpack_19(uint32_t *output, uint32_t *encode, int bitSize);
		static void unpack_20(uint32_t *output, uint32_t *encode, int bitSize);
		static void unpack_21(uint32_t *output, uint32_t *encode, int bitSize);
		static void unpack_22(uint32_t *output, uint32_t *encode, int bitSize);
		static void unpack_23(uint32_t *output, uint32_t *encode, int bitSize);
		static void unpack_24(uint32_t *output, uint32_t *encode, int bitSize);
		static void unpack_25(uint32_t *output, uint32_t *encode, int bitSize);
		static void unpack_26(uint32_t *output, uint32_t *encode, int bitSize);
		static void unpack_27(uint32_t *output, uint32_t *encode, int bitSize);
		static void unpack_28(uint32_t *output, uint32_t *encode, int bitSize);
		static void unpack_29(uint32_t *output, uint32_t *encode, int bitSize);
		static void unpack_30(uint32_t *output, uint32_t *encode, int bitSize);
		static void unpack_31(uint32_t *output, uint32_t *encode, int bitSize);
		static void unpack_32(uint32_t *output, uint32_t *encode, int bitSize);

	private:
		uint8_t m_buf[(P4D_BLOCK_SIZE+1)*8];	//存储压缩的buf
		uint8_t m_unpackBuf[(P4D_BLOCK_SIZE+1)*4];	//解压缩中间buf
		uint8_t m_bitsList[32];			//存储block内元素bit数
		int m_offset;	//buf的偏移量，也就是压缩后的字节数
		uint32_t m_resultList[P4D_BLOCK_SIZE+1];
		static unpackfunc unpacktable[32];
		uint32_t m_lastEncode;
		uint32_t m_lastDecode;
};

}

#endif
