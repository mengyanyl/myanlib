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

#include "threadPool.h"
#include "logger.h"
#include "utils.h"

class PooledThread : public Runnable
{
public:
    PooledThread() {};
    PooledThread(string name);
    ~PooledThread();

    void start();
    void start(Runnable* target);
    void start(Runnable* target, string name);
    bool idle();
    int idleTime();
    void join();
    void activate();
    void release();
    void run();
    void setQueue(BlockingQueue<Runnable *> *aqueue);
private:
    volatile bool        _idle;
    volatile std::time_t _idleTime;
    Runnable* _pTarget;
    boost::thread *_pThread;
    std::string          _name;
    boost::mutex         _mutex;
    boost::condition_variable_any _condTargetFinished;
    BlockingQueue<Runnable *> *_pQueue;
};

PooledThread::PooledThread(std::string name):
    _idle(true),
    _idleTime(0),
    _name(name),
    _pTarget(NULL)
{
    _idleTime = std::time(NULL);
}

PooledThread::~PooledThread()
{
    delete _pThread;
}

void PooledThread::start()
{
    try
    {
        _pThread = new boost::thread(boost::bind(&Runnable::run, this));
    }
    catch (std::exception &e)
    {
        Logger::getLogger().error("create thread error, msg: %s", e.what());
    }
}

void PooledThread::start(Runnable* target)
{
    _pQueue->put(target);
}

void PooledThread::start(Runnable* target, string name)
{
    boost::mutex::scoped_lock lock(_mutex);
    string fullName(name);
    if (name.empty())
    {
        fullName = "";
    }
    else
    {
        fullName.append("(");
        fullName.append(name);
        fullName.append(")");
    }
    _name = fullName;
    _pQueue->put(target);
}

void PooledThread::run()
{
    for (;;)
    {
        //Logger::getLogger().debug("%s thread wait to take", _name.c_str());
        //Logger::getLogger().debug("%s thread queue size: %d", _name.c_str(), _pQueue->getQueueSize());
        _pQueue->take(_pTarget);
        if ( _pTarget )
        {
            _mutex.lock();
            _idle = false;
            _mutex.unlock();
            try
            {
                _pTarget->run();
            }
            catch(std::exception &e)
            {
                Logger::getLogger().error("[thread %s] run error, msg: %s", _name.c_str(), e.what());
            }
            _mutex.lock();
            _idleTime = time(NULL);
            _idle = true;
            _mutex.unlock();

            delete _pTarget;
        }
        else
        {
            Logger::getLogger().debug("%s thread notify running, target is null....", _name.c_str());
            break;
        }
    }
    Logger::getLogger().debug("%s thread stoped", _name.c_str());
    _condTargetFinished.notify_one();
}

inline bool PooledThread::idle()
{
    return _idle;
}

int PooledThread::idleTime()
{
    return (int)(time(NULL) - _idleTime);
}

void PooledThread::join()
{
    _pThread->join();
}

void PooledThread::activate()
{
    boost::mutex::scoped_lock lock(_mutex);
    _idle = false;
}

void PooledThread::release()
{
    boost::mutex::scoped_lock lock(_mutex);
    _idle = true;
    //waiting for 10s, timed out
    _condTargetFinished.timed_wait(lock, boost::get_system_time() + boost::posix_time::seconds(5));
    Logger::getLogger().debug("%s thread release", _name.c_str());
}

void PooledThread::setQueue(BlockingQueue<Runnable *> *aqueue)
{
    _pQueue = aqueue;
}

ThreadPool::ThreadPool(string name,
                       int minCapacity,
                       int maxCapacity,
                       int idleTime) : _name(name), _minCapacity(minCapacity),
    _maxCapacity(maxCapacity), _idleTime(idleTime),  _serial(0)
{
    for (int i = 0; i < _minCapacity; i++)
    {
        std::ostringstream name;
        name << _name << "[#" << ++_serial << "]";
        PooledThread *pt = new PooledThread(name.str());
        pt->setQueue(&_queue);
        _threads.push_back(pt);
        pt->start();
    }
}

void ThreadPool::setCapacity(int n)
{
    boost::mutex::scoped_lock(_mutex);
    if (n <= _minCapacity) return;
    int num = n - _minCapacity;
    for (int i=0; i<num; ++i)
    {
        std::ostringstream name;
        name << _name << "[#" << ++_serial << "]";
        PooledThread *pt = new PooledThread(name.str());
        pt->setQueue(&_queue);
        _threads.push_back(pt);
        pt->start();
    }
    _maxCapacity = (_maxCapacity < n) ? n : _maxCapacity;
}

int ThreadPool::capacity() const
{
    boost::mutex::scoped_lock(_mutex);
    return _maxCapacity;
}

int ThreadPool::available() const
{
    boost::mutex::scoped_lock(_mutex);

    int count = 0;
    ThreadVector::const_iterator iter = _threads.begin();
    for (; iter != _threads.end(); ++iter)
    {
        if ( (*iter)->idle() ) ++count;
    }

    return (_maxCapacity - _threads.size() + count);
}

int ThreadPool::used() const
{
    boost::mutex::scoped_lock(_mutex);

    int count = 0;
    ThreadVector::const_iterator iter = _threads.begin();
    for (; iter != _threads.end(); ++iter)
        if ( !(*iter)->idle() ) ++count;
    return count;
}

int ThreadPool::allocated() const
{
    boost::mutex::scoped_lock(_mutex);
    return _threads.size();
}

PooledThread* ThreadPool::createThread()
{
    std::ostringstream name;
    name << _name << "[#" << ++_serial << "]";
    PooledThread *pt = new PooledThread(name.str());
    return pt;
}

PooledThread* ThreadPool::getThread()
{
    boost::mutex::scoped_lock(_mutex);

    PooledThread *pThread = NULL;

    for (ThreadVector::const_iterator iter = _threads.begin(); !pThread && iter != _threads.end(); ++iter)
    {
        if ( (*iter)->idle() )  pThread = *iter;
    }

    if (!pThread)
    {
        if (_threads.size() < _maxCapacity)
        {
            pThread = this->createThread();
            _threads.push_back(pThread);
            Logger::getLogger().debug("create new thread");
            pThread->start();
        }
        else
            Logger::getLogger().error("thread pool is already max");
    }

    pThread->activate();

    return pThread;
}

void ThreadPool::housekeep()
{
    if (_threads.size() < _minCapacity) return;

    ThreadVector idleThreads;
    ThreadVector expiredThreads;
    ThreadVector activeThreads;
    idleThreads.reserve(_threads.size());
    activeThreads.reserve(_threads.size());

    for (ThreadVector::iterator it = _threads.begin(); it != _threads.end(); ++it)
    {
        if ((*it)->idle())
        {
            if ((*it)->idleTime() < _idleTime)
                idleThreads.push_back(*it);
            else
                expiredThreads.push_back(*it);
        }
        else activeThreads.push_back(*it);
    }
    int n = (int) activeThreads.size();
    int limit = (int) idleThreads.size() + n;
    if (limit < _minCapacity) limit = _minCapacity;
    idleThreads.insert(idleThreads.end(), expiredThreads.begin(), expiredThreads.end());
    _threads.clear();
    for (ThreadVector::iterator it = idleThreads.begin(); it != idleThreads.end(); ++it)
    {
        if (n < limit)
        {
            _threads.push_back(*it);
            ++n;
        }
        else (*it)->release();
    }
    _threads.insert(_threads.end(), activeThreads.begin(), activeThreads.end());
}

void ThreadPool::joinAll()
{
    boost::mutex::scoped_lock(_mutex);

    for (ThreadVector::iterator it = _threads.begin(); it != _threads.end(); ++it)
    {
        (*it)->join();
    }
}

void ThreadPool::start(Runnable* target)
{
    _queue.put(target);
    Logger::getLogger().debug("[thread %s]threadpool queue size: %d", _name.c_str(), _queue.size());
}

void ThreadPool::stopAll()
{
    //put end flag
    for (ThreadVector::iterator it = _threads.begin(); it!=_threads.end(); ++it)
        _queue.put(NULL);

    for (ThreadVector::iterator it = _threads.begin(); it!=_threads.end(); ++it)
    {
        (*it)->release();
    }

    //delete pooledthread
    for (ThreadVector::iterator it = _threads.begin(); it!=_threads.end(); ++it)
    {
        delete *it;
    }
    _threads.clear();
}

class ThreadPoolSingleton
{
public:
    ThreadPoolSingleton():_pool(NULL){};

    ~ThreadPoolSingleton()
    {
        delete _pool;
    }

    ThreadPool& getInstance()
    {
        boost::mutex::scoped_lock lock(_mutex);
        if (!_pool)
        {
            _pool = new ThreadPool("ThreadPool");
        }
        return *_pool;
    }
private:
    ThreadPool *_pool;
    boost::mutex _mutex;
};

static ThreadPoolSingleton threadPoolSingleton;

ThreadPool& ThreadPool::getThreadPool()
{
    return threadPoolSingleton.getInstance();
}
