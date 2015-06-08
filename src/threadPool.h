/*******************************************************************************
   Copyright (C) 2014 MyanDB Software Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.
*******************************************************************************/

#ifndef THREADPOOL_H
#define THREADPOOL_H

#include "core.h"
#include "logger.h"
#include "runnable.h"
#include "blockingQueue.h"

using namespace boost;
using namespace std;
using namespace myan::utils;

/**
thread pool, this pattern reference to poco-1.4(c++ open-source framework)
date: 2014-08-04
**/

class PooledThread;

class ThreadPool
{
public:
    typedef vector<PooledThread*> ThreadVector;
    ThreadPool(string name,
               int minCapacity = 2,
                       int maxCapacity = 5,
                       int idleTime = 60
               );
    virtual ~ThreadPool(){};

    void setCapacity(int n);

    int capacity() const;

    int available() const;

    int used() const;

    int allocated() const;

    void joinAll();

    void start(Runnable* target);

    void stopAll();

    static ThreadPool& getThreadPool();

protected:
private:
    PooledThread* createThread();
    PooledThread* getThread();

    void housekeep();

    string _name;
    int _minCapacity;
    int _maxCapacity;
    int _idleTime;
    int _serial;
    ThreadVector _threads;
    boost::mutex _mutex;
    BlockingQueue<Runnable *> _queue;
};

#endif // THREADPOOL_H
