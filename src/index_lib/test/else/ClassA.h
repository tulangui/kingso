/*
 * ClassA.h
 *
 *  Created on: 2011-5-16
 *      Author: yewang
 */

#ifndef CLASSA_H_
#define CLASSA_H_

#include <stdint.h>


class A
{
public:
    A();
    virtual ~A();

    virtual uint32_t seek(uint32_t docId) = 0;
};



class B : public A
{
public:
    B();
    virtual ~B();

    uint32_t seek(uint32_t docId);

private:
    uint32_t  m_count ;
};


class C : public A
{
public:
    C();
    virtual ~C();

    uint32_t seek(uint32_t docId);

private:
    uint32_t  m_count ;
};



#endif /* CLASSA_H_ */
