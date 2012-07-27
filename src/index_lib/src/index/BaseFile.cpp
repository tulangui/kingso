#include "BaseFile.h"

namespace store {

CBaseFile::CBaseFile() {}
CBaseFile::~CBaseFile() {}

/*
 *	在文件最后开辟一个新的空间， 返回最新开的头位置
 */
int32_t CBaseFile::makeNewPage(int32_t nPageSize)
{
	int32_t nDocPos = static_cast<int32_t>(getSize());	// 得到文件最后位置
	//在文件最后开辟一个新的空间， 把文件移到特定的位置进行写，写什么内容没关系，目的只是把文件撑大
	setPosition(nPageSize-1, SEEK_END); 
	write(&nDocPos, 1);
	return (nDocPos&0x7FFFFFFF);
}

}
