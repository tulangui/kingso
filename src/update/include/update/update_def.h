/*********************************************************************
 * $Author: kingcha $
 *
 * $LastChangedBy: kingcha $
 *
 * $Revision: 2577 $
 *
 * $LastChangedDate: 2011-03-09 09:50:55 +0800 (Wed, 09 Mar 2011) $
 *
 * $Id: update_def.h 2577 2011-03-09 01:50:55Z kingcha $
 *
 * $Brief: ini file parser $
 ********************************************************************/

#ifndef _UPDATE_DEF_H_
#define _UPDATE_DEF_H_

#define UPDATE update 
#define UPDATE_BEGIN namespace update { 
#define UPDATE_END } 

UPDATE_BEGIN

//更新操作定义
enum UpdateAction {
    ACTION_NONE     = 1,
    ACTION_ADD      = 2,
    ACTION_DELETE   = 3,
    ACTION_MODIFY   = 4,
    ACTION_UNDELETE = 5,
    ACTION_LAST     = 6
};

const static int MAX_UPDATE_SERVER_NUM = 128;   //update server的最大数量
const static int DEFAULT_MESSAGE_SIZE = 102400; //默认的一条消息size

UPDATE_END

#endif //_UPDATE_DEF_H_
