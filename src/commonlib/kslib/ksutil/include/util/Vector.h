
/** \file
 *********************************************************************
 * $Author: gongyi.cl $
 *
 * $LastChangedBy: gongyi.cl $
 *
 * $Revision: 478 $
 *
 * $LastChangedDate: 2011-04-08 15:16:21 +0800 (Fri, 08 Apr 2011) $
 *
 * $Id: Vector.h 478 2011-04-08 07:16:21Z gongyi.cl $
 *
 * $Brief: c++动态数组模板类 $
 ********************************************************************/
#ifndef _CPP_VECTOR_H
#define _CPP_VECTOR_H

#include <stdint.h>
#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "namespace.h"

UTIL_BEGIN;

template < typename T > class Vector {
public:

    /**
     * @brief 拷贝构造函数
     *
     * @param array 指向数组的指针
     * @param size  数组长度
     *
     */
    Vector(T *array, uint64_t count){
        _inited = false;
        _size = 1;
        _count = 0;
        _array = NULL;

        while (_size < count) {
            _size *= 2;
        }

        _array = new T[_size];
        if (!_array) {
            return;
        }

        for (uint64_t i = 0; i < count; i++) {
            _array[i] = array[i];
        }

        _count = count;
        _inited = true;
    }

    /**
     * @brief 拷贝函数
     *
     * @param vect 被拷贝的Vector
     */
    Vector(const Vector < T > &vect) {
        this->_inited = false;
        this->_size = vect._size;
        this->_count = vect._count;
        //        this->_array = (T *) malloc(sizeof(T) * _size);
        this->_array = new T[_size];
        if (!this->_array) {
            return;
        }
        for (unsigned int i = 0; i < _count; i++) {
            _array[i] = vect._array[i];
        }

        this->_inited = true;
    }

    /**
     * @brief Vector构造函数
     * @param initSize 制定数组初始大小
     */
    Vector(uint64_t initSize = 64) {
        _inited = false;
        _count = 0;
        if (initSize == 0) {
            initSize = 1;
        }
        _size = initSize;

        //        _array = (T *) malloc(sizeof(T) * _size);
        _array = new T[_size];
        if (!_array) {
            return;                //NULL
        }

        _inited = true;
    }

    /**
     * @brief 析构函数
     */
    virtual ~Vector(void) {
        //        free(_array);
        delete []_array;
        _array = NULL;
        _size = 0;
        _inited = false;
    }

    /**
     * @brief 在数组末尾追加一个元素
     *
     * @param _value插入的元素
     *
     * @return true/false 成功失败.(取决于内存分配是否成功）
     */
    inline bool pushBack(const T & _value) {

        if (!_inited) {
            return false;
        }

        if (_count == 0xFFFFFFFF) {
            return false;
        }

        T *pTmp = NULL;
        if (_count == _size) {
            _size *= 2;
            pTmp = new T[_size];
            //            pTmp = (T *) realloc(_array, sizeof(T) * _size);
            if (!pTmp) {
                return false;
            }
            for (uint64_t i = 0; i < _count; i++) {
                pTmp[i] = _array[i];
            }
            delete []_array;
            _array = pTmp;
        }

        _array[_count] = _value;
        _count++;
        return true;
    }

    /**
      * @brief 将另一个Vector追加至最后。
      *
      * @param vect 另一个Vector
      *
      * @return true/false
      */
     bool pushBack(Vector &vect) {

         if (!_inited || !vect._inited){
             return false;
         }

         if ( (uint64_t)_count + vect._count > 0xFFFFFFFF ) {
             return false;
         }

         bool changed = false;
         while (_size < _count + vect._count) {
             _size *= 2;
             changed = true;
         }

         if (changed) {
             T *pTmp = NULL;
             pTmp = new T[_size];
             if (!pTmp) {
                 return false;
             }

             for (uint64_t i = 0; i < _count; i++) {
                 pTmp[i] = _array[i];
             }
             delete []_array;
             _array = pTmp;
         }

         for (uint64_t i = 0; i < vect._count; i++) {
             _array[_count] = vect._array[i];
             ++_count;
         }

         return true;
     }

     /**
      * @brief 将另一个数组追加至最后。
      *
      * @param vect 另一个Vector
      *
      * @return true/false
      */
     bool pushBack(T *array, uint64_t count) {

         if (!_inited){
             return false;
         }

         if ( (uint64_t)_count + count > 0xFFFFFFFF ) {
             return false;
         }

         bool changed = false;

         while (_size < _count + count) {
             _size *= 2;
             changed = true;
         }

         if (changed) {
             T *pTmp = NULL;
             pTmp = new T[_size];
             if (!pTmp) {
                 return false;
             }

             for (uint64_t i = 0; i < _count; i++) {
                 pTmp[i] = _array[i];
             }
             delete []_array;
             _array = pTmp;
         }

         for (uint64_t i = 0; i < count; i++) {
             _array[_count] = array[i];
             ++_count;
         }

         return true;
     }

    /**
     * @brief 等号运算符重载
     *
     * @param vect 被拷贝的Vector
     *
     * @param Vector<T>类型值
     */
    Vector < T > &operator=(const Vector < T > &vect) {

        if (this == &vect) {
            return *this;
        }

        if (!_inited) {
            return *this;
        }

        _count = vect._count;
        if (_array) {
            if (_size != vect._size) {
                _size = vect._size;
                //free(_array);
                delete []_array;
                //                _array = (T *) malloc(sizeof(T) * _size);
                _array = new T[_size];
            }
            //else{} 不需要重新分配
        }
        else {
            _size = vect._size;
            //            _array = (T *) malloc(sizeof(T) * _size);
            _array = new T[_size];
        }
        if (!_array) {
            return *this;
        }

        for (unsigned int i = 0; i < _size; i++) {
            _array[i] = vect._array[i];
        }
        return *this;
    }

    /**
     * @brief 重载[]操作符
     *
     * @param nPos 元素位置。 不能超过size大小，否则结果不可预知。
     *
     * @return 指定位置的元素
     */
    inline T & operator[] (uint64_t nPos) {
        assert( nPos < _count );
        return _array[nPos];
    }

    /**
     * @brief 重载[]操作符
     *
     * @param nPos 元素位置。 不能超过size大小，否则结果不可预知。
     *
     * @return 指定位置的元素
     */
    inline const T & operator[] (uint64_t nPos) const {
        assert( nPos < _count );
        return _array[nPos];
    }

    /**
     * @brief 清除数组中元素（不会释放内存，仅将元素数量置为0）
     */ void clear() {
         _count = 0;
     }

     /**
      * @brief 删除制定位置的元素
      *
      * @param pos 制定位置。如果n>=元素数量或者元素数为0，不做任何操作。
      */
     void erase(uint64_t pos) {

         if (!_inited) {
             return;
         }

         if ((pos >= _count) || (_count <= 0)) {
             return;
         }

         uint64_t i = 0;
         for (i = pos; i < _count - 1; i++) {
             _array[i] = _array[i + 1];
         }
         --_count;
     }

     /**
      * @brief 获得数组中元素个数
      *
      * @return vector中现有元素的个数
      */
     uint64_t size() const {
         if (!_inited) {
             return 0;
         }
         return _count;
     }

     /**
      * @brief  用一个给定长度的数组，对Vector赋值。
      *
      * @param array 数组指针
      * @param count 数组元素个数
      *
      * @return true/false 返回为false时，Vector中的数据可能已被清除。
      */
     bool assign(const T *array, uint64_t count){
        uint64_t tsize = 1;
        T *pTmp = NULL;

        while (tsize<count) {
            tsize *= 2;
        }

        if (tsize != _size) {
            pTmp = new T[tsize];
            if (!pTmp) {
                return false;
            }
            delete []_array;
            _array = pTmp;
            _size = tsize;
        }

        for (uint64_t i = 0; i < count; i++) {
            _array[i] = array[i];
        }

        _count = count;

        return true;
     }

     /**
      * @brief  用一个给定长度的数组，对Vector赋值。
      *
      * @param array 数组指针
      * @param count 数组元素个数
      *
      * @return true/false 返回为false时，Vector中的数据可能已被清除。
      */
     bool assign(const Vector &vect){
         if (!vect.isInited()) {
             return false;
         }
         return assign(vect._array, vect._count);
     }

     /**
      * @brief 返回Vector是否被初始化
      *
      * @return true/false
      */
     bool isInited() const {
         return _inited;
     }

     typedef T value_type;
     typedef value_type* iterator;

     /**
      * @brief 返回Vector迭代器begin位置
      *
      * @return iterator
      */
     iterator begin() const {
         return _array;
     }

     /**
      * @brief 返回Vector结束位置（不可访问）

      * @return iterator
      */
     iterator end() const {
         return _array+_count; //不可访问！
     }


     /**
      * @brief 调试接口，打印内部信息到fp
      *
      * @param fp 输出的 fp
      */
     void __print_info(FILE *fp){
         assert(fp);
         fprintf(fp, "%s\n", _inited?"Inited.":"Not inited.");
         fprintf(fp, "Array Size: %llu\n", (unsigned long long)_size);
         fprintf(fp, "Item Count: %llu\n", (unsigned long long)_count);
     }

     /**
      * @brief 返回最后一个元素
      * @return 最后一个元素的值。
      */
     inline T& back() {
        assert(_count > 0);
        return _array[_count-1];
     }

     /**
      * @brief 删除最后一个元素。
      */
     inline void popBack(){
         assert(_count > 0);
         _count--;
         return;
     }

private:
     uint64_t _size;
     uint64_t _count;
     T *_array;
     bool _inited;
};

UTIL_END;


#endif
