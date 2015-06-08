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

#ifndef _BLOCKINGQUEUE_H_
#define _BLOCKINGQUEUE_H_

#include "core.h"
#include "logger.h"

namespace myan
{
namespace utils
{

template <class T>
class BlockingQueue
{
public:
    BlockingQueue(std::string aqueueName="default", int maxCount=50)
    {
        _queueName= aqueueName;
        _maxCount=maxCount;
        ready=true;
        _queueCount=0;
    }

    void setQueueCount(int count)
    {
        this->_queueCount= count;
    }

    virtual ~BlockingQueue<T>(void)
    {
    }

    bool offer(T t)
    {
        boost::mutex::scoped_lock scope(_mutex);

        if (_queueCount>=_maxCount)
        {
            //Myan::NewFile::Logger::getLogger().debug("Queue[%s] is full; count=%d; waiting...", _queueName.c_str(), _queueCount);
            return false;
        }

        _queue.push(t);

        ++_queueCount;

        _cond.notify_one();

        return true;
    }

    void put(T t)
    {
        boost::mutex::scoped_lock scope(_mutex);

        while(_queueCount>=_maxCount)
        {
//            boost::thread::yield();
            //Myan::NewFile::Logger::getLogger().debug("Queue[%s] is full; count=%d; waiting...", _queueName.c_str(), _queueCount);
            _cond.wait(scope);
        }

        _queue.push(t);

        ++_queueCount;

        _cond.notify_one();
    }

    bool poll(T &t)
    {
        boost::mutex::scoped_lock scope(_mutex);

        if (_queue.empty())
        {
            return false;
        }


        t = _queue.front();
        _queue.pop();

        --_queueCount;

        _cond.notify_one();

        return true;
    }

    bool take(T &t)
    {
        boost::mutex::scoped_lock scope(_mutex);
        //std::cout << _queueName << " queue size: " <<  _queueCount << "*****************" << std::endl;
        while(_queue.empty())
        {
//            boost::thread::yield();
            _cond.wait(scope);
        }

        t = _queue.front();
        _queue.pop();

        --_queueCount;

        _cond.notify_one();

        return true;
    }

    int getQueueSize()
    {
        boost::mutex::scoped_lock scope(_mutex);
        return _queueCount;
    }

private:
    int _maxCount;
    std::string _queueName;
    int _queueCount;
    bool ready;
    std::queue<T> _queue;
    boost::condition_variable_any _cond;
    boost::mutex _mutex;
};

}
}

#endif

