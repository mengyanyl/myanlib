#ifndef UTILS_H_
#define UTILS_H_

#include <ctime>
#include <zlib.h>
#include <sys/time.h>
#include <stdarg.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <iostream>
#include <stdint.h>
#include <string.h>



namespace myan
{
namespace utils
{

uint32_t getTickCount();

uint64_t getTickCount64();

size_t formatTimeToString(std::string& value);

size_t formatTimeToString(std::string& value);

int safe_sprintf(std::string& buf, const char * fmt, ...);

int safe_vsprintf(std::string& buf, const char * fmt, va_list argptr);

uint32_t unzip_data(void * source, uint32_t source_size, char *buffer, uint32_t buffer_size);

uint32_t zip_data(void* source, uint32_t source_size, char *buffer, uint32_t buffer_size);

void	ltrim_chars(char *str, const char *charlist);

void	rtrim_chars(char *str, const char *charlist);

void	ltrim_spaces(char *c);

void	rtrim_spaces(char *c);

void	lrtrim_spaces(char *c);

std::string getExeFileName();

std::string getExeFilePath();

int createPIDFile(const char* fileName);

int dropPIDFile(const char* fileName);

int splitString(std::string &str, const char* pDelimiter, std::vector<std::string> &strVec);

void toLower(std::string &s);

void toUpper(std::string &s);

}
}


#endif // UTILS_H_
