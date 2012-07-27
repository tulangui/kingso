/*
 * ClassA.cpp
 *
 *  Created on: 2011-5-16
 *      Author: yewang
 */

#include "ClassA.h"


A::A() {}
B::B() {}
C::C() {}

A::~A() {}
B::~B() {}
C::~C() {}


uint32_t B::seek(uint32_t docId)
{
    m_count++;

    return m_count;
}



uint32_t C::seek(uint32_t docId)
{
    m_count++;

    return m_count;
}
