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

#include "logger.h"

namespace myan
{
namespace utils
{

static const char * LOG_LEVEL_NAME_ARRAY[] = { "DEBUG", "INFO", "WARN", "ERROR",
        "FATAL"
                                             };

Logger::Logger():_showConsole(false), _haveConsole(false),_isStoped(false),_logLevel(0)
{
    //为什么在构造函数中初始化成员不行，_isStoped初始为false，
    //但是在调用info是_isStoped为true，是应为LoggerSingleton类的问题吗？
    //所以现在初始化放在init方法中
}

Logger::~Logger()
{
    this->release();
    std::cout << "delete logger thread" << std::endl;
}

Logger::Logger(const Logger& other)
{
}

void Logger::init(const char *pFileName, int logLevel, bool isConsole)
{
    _isStoped=false;
    _logLevel=0    ;
    memset(_fileName, 0, sizeof(_fileName));
    memset(_errFileName, 0, sizeof(_errFileName));
    memset(_fileExtName, 0, sizeof(_fileExtName));
    memset(_filePath, 0, sizeof(_filePath));

    _logLevel = logLevel;
    _showConsole = isConsole;

    struct stat st;

    int fd = fileno(stdout);
    if (fd == -1 || -1 == fstat(fd, &st))
    {
        printf("no stdout exist.\n");
        _haveConsole = false;
    }
    else
    {
        _haveConsole = true;
    }

    //get file path
    const char *p = strrchr(pFileName, int('/'));
    if (p)
    {
        strncpy(_filePath, pFileName, p-pFileName+1);
    }
    //get file name
    if (p)
        strncpy(_fileName, p + 1, strlen(pFileName) - (p + 1 - pFileName) );
    else
        strncpy(_fileName, pFileName, strlen(pFileName));
    //get file ext name
    p = strrchr(_fileName, '.');
    if (p)
        strcpy(_fileExtName, p);
    else
        strcpy(_fileExtName, ".log");

    strcpy(_errFileName, "ERROR_");
    strcat(_errFileName, _fileName);

    boost::thread _thread(boost::bind(&Logger::run, this));
    //printf("logger thread start......\n");
}

void Logger::run(void)
{
    std::string *pMsg=0;
    PLOGITEM pItem = 0;

    while(true)
    {
        if (_queue.empty() && _isStoped)
        {
            break;
        }
        else if (_queue.empty() && !_isStoped)
        {
            continue;
        }
        else if (!_queue.empty())
        {
            boost::mutex::scoped_lock lock(_fastMutex);

            //得到对象指针
            pItem = this->_queue.front();
            this->_queue.pop();
            if (pItem->sec == 0 && pItem->msec == 0 && pItem->msg_size == 0)
            {
                break;
            }

            try
            {
                writeLogItem(pItem);
            }
            catch(...)
            {

            }

            //释放对象
            delete pItem;
        }
    }
    _cond.notify_one();
}

int Logger::writeLogItem(PLOGITEM pItem)
{
    tm * tm, tmbuf;
    tm = localtime(&pItem->sec);


    std::string filename;
    safe_sprintf(filename, "%s%s_%04d%02d%02d%s", _filePath, _fileName, tm->tm_year
                              + 1900, tm->tm_mon + 1, tm->tm_mday, _fileExtName);
    FILE* fp = fopen(filename.c_str(), "a+");
    if (NULL == fp)
    {
        // 出错了。
        return -1;
    }

    fprintf(fp, "[%02d:%02d:%02d.%03d %s] ", tm->tm_hour,
            tm->tm_min, tm->tm_sec, pItem->msec,
            LOG_LEVEL_NAME_ARRAY[pItem->level]);
    fwrite(pItem->msg, pItem->msg_size, 1, fp);
    fprintf(fp, "\n");
    fclose(fp);

    if (_haveConsole && _showConsole)
    {
        fprintf(stdout, "[%02d:%02d:%02d.%03d %s] ", tm->tm_hour,
                tm->tm_min, tm->tm_sec, pItem->msec,
                LOG_LEVEL_NAME_ARRAY[pItem->level]);
        fwrite(pItem->msg, pItem->msg_size, 1, stdout);
        fprintf(stdout, "\n");
    }

    if (pItem->level >= LOG_ERROR_LEVEL)
    {
        // 错误日志专门写个文件。
        std::string err_filename;
        safe_sprintf(err_filename, "%s%s_%04d%02d%02d%s", _filePath, _errFileName, tm->tm_year
                                  + 1900, tm->tm_mon + 1, tm->tm_mday, _fileExtName);
        FILE * errfp = fopen(err_filename.c_str(), "a+");
        if (errfp)
        {
            fprintf(errfp, "[%02d:%02d:%02d.%03d %s] ", tm->tm_hour,
                    tm->tm_min, tm->tm_sec, pItem->msec,
                    LOG_LEVEL_NAME_ARRAY[pItem->level]);
            fwrite(pItem->msg, pItem->msg_size, 1, errfp);
            fprintf(errfp, "\n");
            fclose(errfp);
        }
    }
}

int Logger::writeV(int logLevel, char* fmt, va_list argptr)
{
    _isStoped=0;
    if (_isStoped) return 0;

    char *pBuf = new char[LOG_MAX_MSG_SIZE + sizeof(LOGITEM) + 8];
    memset(pBuf, 0, sizeof(pBuf));
    PLOGITEM pItem = (PLOGITEM)pBuf;

    //ptime ptie(microsec_clock::local_time());
    //ptime ptis(date(1970,1,1), time_duration(0,0,0));
    //time_duration td = ptie - ptis;
    //pItem->sec = td.ticks()/td.ticks_per_second();
    //pItem->msec = td.fractional_seconds();
    timeval tv;
    gettimeofday(&tv, NULL);
    pItem->sec=tv.tv_sec;
    pItem->msec=(int)(tv.tv_usec / 1000);
    pItem->level = logLevel;

    size_t msg_size = vsnprintf(pItem->msg, LOG_MAX_MSG_SIZE, fmt, argptr);

    if (msg_size<=0)
    {
        return 0;
    }

    if (msg_size>LOG_MAX_MSG_SIZE) msg_size=LOG_MAX_MSG_SIZE-1;
    pItem->msg_size = msg_size;
    size_t size = pItem->msg_size + sizeof(pItem);

    this->_fastMutex.lock();
    this->_queue.push(pItem);
    this->_fastMutex.unlock();

    return 1;
}

int Logger::info(char *fmt, ...)
{
    if (Logger::LOG_INFO_LEVEL < _logLevel)
    {
        return 0;
    }

    va_list argptr;
    va_start(argptr, fmt);

    int ret = this->writeV(LOG_INFO_LEVEL, fmt, argptr);

    va_end(argptr);

    return ret;
}

int Logger::safe_sprintf(std::string& buf, const char * fmt, ...)
{
    va_list argptr;
    va_start(argptr, fmt);

    int ret = safe_vsprintf(buf, fmt, argptr);

    va_end(argptr);

    return ret;
}

int Logger::safe_vsprintf(std::string& buf, const char * fmt, va_list argptr)
{
    char buffer[1024];

    int ret = ::vsnprintf(buffer, sizeof(buffer), fmt, argptr);
    if (ret >= (int) sizeof(buffer))
    {
        if (ret >= (int) buf.max_size())
        {
            buf.clear();
            printf("safe_vsprintf: string max_size limit(%d)", buf.max_size());
            return 0;
        }
        buf.resize(ret);
        ret = vsnprintf(&buf.at(0), ret + 1, fmt, argptr);
    }
    else
    {
        buf = buffer;
    }
    //va_end(argptr);
    return ret;
}

int Logger::debug(char *fmt, ...)
{
    if (Logger::LOG_DEBUG_LEVEL < _logLevel)
    {
        return 0;
    }

    va_list argptr;
    va_start(argptr, fmt);

    int ret = this->writeV(LOG_DEBUG_LEVEL, fmt, argptr);

    va_end(argptr);

    return ret;
}

int Logger::error(char *fmt, ...)
{
    if (Logger::LOG_ERROR_LEVEL < _logLevel)
    {
        return 0;
    }

    va_list argptr;
    va_start(argptr, fmt);

    int ret = this->writeV(LOG_ERROR_LEVEL, fmt, argptr);

    va_end(argptr);

    return ret;
}

int Logger::warn(char *fmt, ...)
{
    if (Logger::LOG_WARN_LEVEL < _logLevel)
    {
        return 0;
    }

    va_list argptr;
    va_start(argptr, fmt);

    int ret = this->writeV(LOG_WARN_LEVEL, fmt, argptr);

    va_end(argptr);

    return ret;
}

void Logger::release()
{
    _isStoped=true;
    boost::mutex::scoped_lock lock(_fastMutex);
    _cond.wait(lock);
}

void Logger::setLevel(int logLevel)
{
    this->_logLevel = logLevel;
}

int Logger::getLevel()
{
    return _logLevel;
}

/**
Logger单例类
**/
class LoggerSingleton
{
public:
    LoggerSingleton():_pLogger(NULL) {}
    ~LoggerSingleton()
    {
        delete _pLogger;
    }
    //得到Logger
    Logger *getLogger()
    {
        //需要同步,应用于多线程
        boost::mutex::scoped_lock lock(_mutex);
        if (_pLogger==NULL)
            _pLogger = new Logger();

        return _pLogger;
    }
private:
    Logger *_pLogger;
    boost::mutex _mutex;
};


static LoggerSingleton logSingleton;

//用此静态方法得到Logger，
Logger& Logger::getLogger()
{
    return *logSingleton.getLogger();
}

}
}
