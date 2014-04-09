/** \file
 * \author Max Mertens <max.mail@dameweb.de>
 * \section LICENSE
 * Copyright (c) 2014, Max Mertens.
 * All rights reserved.
 * This file is licensed under the "BSD 3-Clause License".
 * Full license text is under the file "LICENSE" provided with this code. */

#ifndef _TWODIMARRAY_HPP_
#define _TWODIMARRAY_HPP_

#include "Console.hpp"

#include <vector>

namespace SphereSim
{
    class ArrayException : public std::exception
    {
        const char* what() const noexcept
        {
            return "Array exception.";
        }
    };


    template <typename T, bool extremeSpeed=true, bool throwExceptions=true>
    class TwoDimArray
    {
    private:
        std::vector<T> values;

        T** subArrays;

        unsigned int* counter;

        const unsigned int constInnerSize;

        unsigned int outerSize;

        void deleteAll()
        {
            if (counter != nullptr)
            {
                delete[] counter;
                counter = nullptr;
            }
            if (subArrays != nullptr)
            {
                delete[] subArrays;
                subArrays = nullptr;
            }
        }

        void initArrays()
        {
            unsigned int totalSize = constInnerSize * outerSize;
            values.resize(totalSize);
            subArrays = new T*[outerSize];
            counter = new unsigned int[outerSize];
            for (unsigned int i = 0; i<outerSize; i++)
            {
                counter[i] = 0;
                subArrays[i] = &values[constInnerSize*i];
            }
        }

    public:
        TwoDimArray(const unsigned int size)
            :values(0), subArrays(nullptr), counter(nullptr),
            constInnerSize(size), outerSize(0)
        {
        }

        TwoDimArray(const unsigned int size, const unsigned int size2)
            :values(0), subArrays(nullptr), counter(nullptr),
            constInnerSize(size), outerSize(size2)
        {
            if (size2 > 0)
            {
                initArrays();
            }
        }

        ~TwoDimArray()
        {
            deleteAll();
        }

        TwoDimArray() = delete;
        TwoDimArray(const TwoDimArray&) = delete;
        TwoDimArray& operator=(const TwoDimArray&) = delete;

        void resize(const unsigned int size)
        {
            if (size != outerSize && size > 0)
            {
                deleteAll();
                outerSize = size;
                values.resize(size);
                initArrays();
            }
        }

        inline T* operator[](const unsigned int index)
        {
            if (extremeSpeed == false)
            {
                if (index >= outerSize || subArrays == nullptr
                    || subArrays[index] == nullptr)
                {
                    Console()<<"TwoDimArray::operator[] error.\n";
                    if (throwExceptions)
                    {
                        throw ArrayException();
                    }
                    return nullptr;
                }
            }
            return subArrays[index];
        }

        inline const T* operator[](const unsigned int index) const
        {
            if (extremeSpeed == false)
            {
                if (index >= outerSize || subArrays == nullptr
                    || subArrays[index] == nullptr)
                {
                    Console()<<"TwoDimArray::operator[] const error.\n";
                    if (throwExceptions)
                    {
                        throw ArrayException();
                    }
                    return nullptr;
                }
            }
            return subArrays[index];
        }

        inline void addElement(const unsigned int index, T& element)
        {
            if (extremeSpeed == false)
            {
                if (counter == nullptr || index >= outerSize || subArrays == nullptr
                    || subArrays[index] == nullptr)
                {
                    Console()<<"TwoDimArray::addElement error.\n";
                    if (throwExceptions)
                    {
                        throw ArrayException();
                    }
                    return;
                }
            }
            if (counter[index] < constInnerSize)
            {
                subArrays[index][counter[index]++] = element;
            }
            else
            {
                Console()<<"TwoDimArray::addElement error: full.\n"<<counter[index]
                    <<constInnerSize;
                if (throwExceptions)
                {
                    throw ArrayException();
                }
            }
        }

        inline bool addElementIfNotContained(const unsigned int index, T& element)
        {
            if (extremeSpeed == false)
            {
                if (counter == nullptr || index >= outerSize || subArrays == nullptr
                    || subArrays[index] == nullptr)
                {
                    Console()<<"TwoDimArray::addElementIfNotContained error.\n";
                    if (throwExceptions)
                    {
                        throw ArrayException();
                    }
                    return false;
                }
            }
            if (counter[index] < constInnerSize)
            {
                T* subArray = subArrays[index];
                for (unsigned int i = 0; i<counter[index]; ++i)
                {
                    if (subArray[i] == element)
                    {
                        return false;
                    }
                }
                subArray[counter[index]++] = element;
                return true;
            }
            Console()<<"TwoDimArray::addElementIfNotContained error: full.\n";
            if (throwExceptions)
            {
                throw ArrayException();
            }
            return false;
        }

        inline unsigned int count()
        {
            return outerSize;
        }

        inline unsigned int getCounter(const unsigned int index)
        {
            if (extremeSpeed == false)
            {
                if (counter == nullptr || index >= outerSize)
                {
                    Console()<<"TwoDimArray::getCount error.\n";
                    if (throwExceptions)
                    {
                        throw ArrayException();
                    }
                    return 0;
                }
            }
            return counter[index];
        }

        inline void resetCounter(const unsigned int index)
        {
            if (extremeSpeed == false)
            {
                if (counter == nullptr || index >= outerSize)
                {
                    Console()<<"TwoDimArray::resetCounter error.\n";
                    if (throwExceptions)
                    {
                        throw ArrayException();
                    }
                    return;
                }
            }
            counter[index] = 0;
        }

        inline T* data()
        {
            return values;
        }

        inline const T* constData() const
        {
            return values;
        }

    };
}

#endif
