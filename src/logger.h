/******************************************************************************
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

#ifndef _LOGGER_H
#define _LOGGER_H

#include "core.h"
#include <boost/thread.hpp>
#include <queue>

using namespace std;

namespace myan
{
namespace utils
{

class Logger
{
public:
// 一行日志内容的最大容量
    static const size_t LOG_MAX_MSG_SIZE = 1024;
// 一行日志内容的最大容量
    static const size_t LOG_MAX_BUFFER_SIZE = 4096 * LOG_MAX_MSG_SIZE;
    static const int LOG_DEBUG_LEVEL	= 0;
    static const int LOG_INFO_LEVEL	= 1;
    static const int LOG_WARN_LEVEL	= 2;
    static const int LOG_ERROR_LEVEL	= 3;
    static const int LOG_FATAL_LEVEL	= 4;

    Logger();
    virtual ~Logger();
    Logger(const Logger& other);

    //静态方法得到Logger单例，此方法线程安全
    static Logger& getLogger();

    void init(const char *pFileName, int logLevel, bool isConsole);

    void run(void);

    void release();

    int info(char *fmt, ...);

    int debug(char *fmt, ...);

    int warn(char *fmt, ...);

    int error(char *fmt, ...);

    int getLevel();

    void setLevel(int logLevel);
private:

    typedef struct LogItem
    {
        int level;
        time_t sec;
        int msec;
        size_t msg_size;
        char msg[0];
    } LOGITEM, *PLOGITEM;
    int writeV(int logLevel, char* fmt, va_list argptr);
    int writeLogItem(PLOGITEM pItem);
    int safe_sprintf(std::string& buf, const char * fmt, ...);
    int safe_vsprintf(std::string& buf, const char * fmt, va_list argptr);

    char _fileName[100];
    char _errFileName[100];
    char _fileExtName[20];
    char _filePath[200];
    bool _isStoped;
    bool _haveConsole;
    bool _showConsole;
    int _logLevel;
    queue<PLOGITEM> _queue;
    boost::mutex _fastMutex;
    boost::condition_variable_any _cond;
};

}
}

#endif // MYANLOGGER_H
