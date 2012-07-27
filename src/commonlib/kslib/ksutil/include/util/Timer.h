/**
 * @file    Timer.h
 * @author  王炜 <santai.ww@taobao.com>
 * @version 1.0
 * @section Date:           2011-03-08
 * @section Description:    基础库中有关时间的基础类和相关函数。 
 * @section Others:         
 * @section Function List:  
 *          1. class Timer: 时间功能类, 包含3个static方法和6个成员方法.
 * @section Copyright (C), 2000-2010, Taobao Tech. Co., Ltd.
 */

#ifndef _UTIL_TIMER_H
#define _UTIL_TIMER_H

#include <stdint.h>
#include <sys/time.h>
#include "namespace.h"

UTIL_BEGIN;

class Timer
{

public:

   /**
    * A constructor.
    */
    Timer();

   /**
    * A destructor.
    */
    ~Timer();

public:

   /**
    * Function:       getTime
    * Description:    获取当前时间，单位：秒
    * Input:          void
    * Output:         返回值
    * @return         当前时间，类型：uint64_t。
    * Usage:          uint64_t beginTime = Timer::getTime();
    */
    static uint64_t getTime();

   /**
    * Function:       getMilliTime
    * Description:    获取当前时间，单位：毫秒
    * Input:          void
    * Output:         返回值
    * @return         当前时间，类型：uint64_t。
    * Usage:          uint64_t beginTime = Timer::getMilliTime();
    */
    static uint64_t getMilliTime();

   /**
    * Function:       getMicroTime
    * Description:    获取当前时间，单位：微秒
    * Input:          void
    * Output:         返回值
    * @return         当前时间，类型：uint64_t。
    * Usage:          uint64_t beginTime = Timer::getMicroTime();
    */
    static uint64_t getMicroTime();

public:

   /**
    * Function:       startTimer
    * Description:    停表计时功能，获取开始计时和停止计时之间经过的时间，单位：秒
    * @see            stopTimer()
    * Input:          void
    * Output:         stopTimer() 的返回值
    * @return         stopTimer() 的返回值：最近一次 startTimer() 后到 stopTimer() 为止经过的时间，类型：uint64_t。
    * Usage:          timer1 和 timer2 的计时之间不会互相影响。
    *                 UTIL::Timer timer1, timer2;
    *                 timer1.startTimer();
    *                 timer2.startTimer();
    *                 ... // some Elapsed Time
    *                 uint64_t uElapsedTimer1 = timer1.stopTimer();
    *                 uint64_t uElapsedTimer2 = timer2.stopTimer();                    
    */
    void startTimer();

   /**
    * Function:       stopTimer
    * Description:    停表计时功能，获取开始计时和停止计时之间经过的时间，单位：秒
    * @see            startTimer()
    * Input:          void
    * Output:         stopTimer() 的返回值
    * @return         stopTimer() 的返回值：最近一次 startTimer() 后到 stopTimer() 为止经过的时间，类型：uint64_t。
    * Usage:          timer1 和 timer2 的计时之间不会互相影响。
    *                 UTIL::Timer timer1, timer2;
    *                 timer1.startTimer();
    *                 timer2.startTimer();
    *                 ... // some Elapsed Time
    *                 uint64_t uElapsedTimer1 = timer1.stopTimer();
    *                 uint64_t uElapsedTimer2 = timer2.stopTimer();                    
    */
    uint64_t stopTimer();

   /**
    * Function:       startMilliTimer
    * Description:    停表计时功能，获取开始计时和停止计时之间经过的时间，单位：毫秒
    * @see            stopMilliTimer()
    * Input:          void
    * Output:         stopMilliTimer() 的返回值
    * @return         stopMilliTimer() 的返回值：最近一次 startMilliTimer() 后到 stopMilliTimer() 为止经过的时间，类型：uint64_t。
    * Usage:          timer1 和 timer2 的计时之间不会互相影响。
    *                 UTIL::Timer timer1, timer2;
    *                 timer1.startMilliTimer();
    *                 timer2.startMilliTimer();
    *                 ... // some Elapsed Time
    *                 uint64_t uElapsedTimer1 = timer1.stopMilliTimer();
    *                 uint64_t uElapsedTimer2 = timer2.stopMilliTimer();                    
    */
    void startMilliTimer(); 

   /**
    * Function:       stopMilliTimer
    * Description:    停表计时功能，获取开始计时和停止计时之间经过的时间，单位：毫秒
    * @see            startMilliTimer()
    * Input:          void
    * Output:         stopMilliTimer() 的返回值
    * @return         stopMilliTimer() 的返回值：最近一次 startMilliTimer() 后到 stopMilliTimer() 为止经过的时间，类型：uint64_t。
    * Usage:          timer1 和 timer2 的计时之间不会互相影响。
    *                 UTIL::Timer timer1, timer2;
    *                 timer1.startMilliTimer();
    *                 timer2.startMilliTimer();
    *                 ... // some Elapsed Time
    *                 uint64_t uElapsedTimer1 = timer1.stopMilliTimer();
    *                 uint64_t uElapsedTimer2 = timer2.stopMilliTimer();                    
    */
    uint64_t stopMilliTimer();

   /**
    * Function:       startMicroTimer
    * Description:    停表计时功能，获取开始计时和停止计时之间经过的时间，单位：微秒
    * @see            stopMicroTimer()
    * Input:          void
    * Output:         stopMicroTimer() 的返回值
    * @return         stopMicroTimer() 的返回值：最近一次 startMicroTimer() 后到 stopMicroTimer() 为止经过的时间，类型：uint64_t。
    * Usage:          timer1 和 timer2 的计时之间不会互相影响。
    *                 UTIL::Timer timer1, timer2;
    *                 timer1.startMicroTimer();
    *                 timer2.startMicroTimer();
    *                 ... // some Elapsed Time
    *                 uint64_t uElapsedTimer1 = timer1.stopMicroTimer();
    *                 uint64_t uElapsedTimer2 = timer2.stopMicroTimer();                    
    */
    void startMicroTimer(); 

   /**
    * Function:       stopMicroTimer
    * Description:    停表计时功能，获取开始计时和停止计时之间经过的时间，单位：微秒
    * @see            startMicroTimer()
    * Input:          void
    * Output:         stopMicroTimer() 的返回值
    * @return         stopMicroTimer() 的返回值：最近一次 startMicroTimer() 后到 stopMicroTimer() 为止经过的时间，类型：uint64_t。
    * Usage:          timer1 和 timer2 的计时之间不会互相影响。
    *                 UTIL::Timer timer1, timer2;
    *                 timer1.startMicroTimer();
    *                 timer2.startMicroTimer();
    *                 ... // some Elapsed Time
    *                 uint64_t uElapsedTimer1 = timer1.stopMicroTimer();
    *                 uint64_t uElapsedTimer2 = timer2.stopMicroTimer();                    
    */
    uint64_t stopMicroTimer();

private:
    struct timeval m_Timer;         // 内部使用，开始计时时刻，用于秒计时器。
    struct timeval m_MilliTimer;    // 内部使用，开始计时时刻，用于毫秒计时器。
    struct timeval m_MicroTimer;    // 内部使用，开始计时时刻，用于微秒计时器。
};

UTIL_END;

#endif
