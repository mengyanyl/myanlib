#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>


#include "utils.h"


namespace myan
{
namespace utils
{

//-----------------------implement------------------------------------//
//得到当前毫秒数
uint32_t getTickCount()
{
    timeval tv;
    gettimeofday(&tv, NULL);
    return (uint32_t)tv.tv_sec * 1000 + (uint32_t)tv.tv_usec/1000;
}

uint64_t getTickCount64()
{
    timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000ll * 1000ll + tv.tv_usec;
}

//将time_t格式化为yyyy-mm-dd hh:mm:ss
size_t formatTimeToString(std::string& value)
{
    if (value.length()<20) return -1;

    time_t t = time(NULL);
    struct tm* ptm = localtime(&t);

    return strftime(&value.at(0), value.length(), "%Y-%m-%d %H:%M:%S", ptm);
}

//将yyyy-mm-dd hh:mm:ss格式化为time_t
time_t formatStringToTime(std::string& value)
{
    struct tm tm;
    char *re = strptime(value.c_str(), "%Y-%m-%d %H:%M:%S", &tm);
    if (re==NULL) return (time_t)-1;
    return mktime(&tm);
}

//压缩数据
uint32_t zip_data(void* source, uint32_t source_size, char *buffer, uint32_t buffer_size)
{
    z_stream def;

    def.zalloc = Z_NULL;
    def.zfree = Z_NULL;
    def.opaque = Z_NULL;

    int ret = deflateInit(&def, Z_DEFAULT_COMPRESSION);
    if (ret != Z_OK)
    {
        std::cerr << "zip_data deflateInit failed" << std::endl;
        return 0;
    }

    /* compress from stdin until output full, or no more input */
    def.avail_out = buffer_size;
    def.next_out = reinterpret_cast<Bytef*>(buffer);

    def.avail_in = source_size;
    def.next_in = reinterpret_cast<Bytef*>(source);
    ret = deflate(&def, Z_FINISH);

    uint32_t bytes = buffer_size - def.avail_out;

    if (Z_STREAM_END != ret)
    {
        std::cerr << "zip_data deflate failed" << std::endl;
        bytes = 0;
    }
    ret = deflateEnd(&def);
    return bytes;
}

//解压数据
uint32_t unzip_data(void * source, uint32_t source_size, char *buffer, uint32_t buffer_size)
{
    int ret;
    z_stream strm;

    /* allocate inflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;

    ret = inflateInit(&strm);
    if (ret != Z_OK)
    {
        std::cerr << "unzip_data inflateInit failed" << std::endl;
        return 0;
    }

    strm.avail_in = source_size;
    strm.next_in = reinterpret_cast<Bytef *>(source);

    strm.avail_out = buffer_size;
    strm.next_out = reinterpret_cast<Bytef*>(buffer);

    ret = inflate(&strm, Z_NO_FLUSH);

    uint32_t bytes = buffer_size - strm.avail_out;

    if (ret != Z_STREAM_END)
    {
        std::cerr << "unzip_data inflate failed" << std::endl;
        bytes = 0;
    }

    /* clean up and return */
    (void)inflateEnd(&strm);
    return bytes;
}

int safe_sprintf(std::string& buf, const char * fmt, ...)
{
    va_list argptr;
    va_start(argptr, fmt);

    int ret = safe_vsprintf(buf, fmt, argptr);

    va_end(argptr);

    return ret;
}

int safe_vsprintf(std::string& buf, const char * fmt, va_list argptr)
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

/******************************************************************************
 *                                                                            *
 * Function: str_ltrim                                                        *
 *                                                                            *
 * Purpose: Strip haracters from the beginning of a string                    *
 *                                                                            *
 * Parameters: str - string to processing                                     *
 *             charlist - null terminated list of characters                  *
 *                                                                            *
 * Return value: Stripped string                                              *
 *                                                                            *
 * Author: Eugene Grigorjev                                                   *
 *                                                                            *
 * Comments:                                                                  *
 *                                                                            *
 ******************************************************************************/
void ltrim_chars(char *str, const char *charlist)
{
    char *p;

    if( !str || !charlist || !*str || !*charlist ) return;

    for( p = str; *p && NULL != strchr(charlist,*p); p++ );

    if( p == str )	return;

    while( *p )
    {
        *str = *p;
        str++;
        p++;
    }

    *str = '\0';
}

/******************************************************************************
 *                                                                            *
 * Function: str_rtrim                                                        *
 *                                                                            *
 * Purpose: Strip haracters from the end of a string                          *
 *                                                                            *
 * Parameters: str - string to processing                                     *
 *             charlist - null terminated list of characters                  *
 *                                                                            *
 * Return value: Stripped string                                              *
 *                                                                            *
 * Author: Eugene Grigorjev                                                   *
 *                                                                            *
 * Comments:                                                                  *
 *                                                                            *
 ******************************************************************************/
void	rtrim_chars(char *str, const char *charlist)
{
    char *p;

    if( !str || !charlist || !*str || !*charlist ) return;

    for(
        p = str + strlen(str) - 1;
        p >= str && NULL != strchr(charlist,*p);
        p--)
        *p = '\0';
}

/******************************************************************************
 *                                                                            *
 * Function: rtrim_spaces                                                     *
 *                                                                            *
 * Purpose: delete all right spaces for the string                            *
 *                                                                            *
 * Parameters: c - string to trim spaces                                      *
 *                                                                            *
 * Return value: string without right spaces                                  *
 *                                                                            *
 * Author: Alexei Vladishev                                                   *
 *                                                                            *
 * Comments:                                                                  *
 *                                                                            *
 ******************************************************************************/
void	rtrim_spaces(char *c)
{
    int i,len;

    len = (int)strlen(c);
    for(i=len-1; i>=0; i--)
    {
        if( c[i] == ' ')
        {
            c[i]=0;
        }
        else	break;
    }
}

/******************************************************************************
 *                                                                            *
 * Function: ltrim_spaces                                                     *
 *                                                                            *
 * Purpose: delete all left spaces for the string                             *
 *                                                                            *
 * Parameters: c - string to trim spaces                                      *
 *                                                                            *
 * Return value: string without left spaces                                   *
 *                                                                            *
 * Author: Alexei Vladishev                                                   *
 *                                                                            *
 * Comments:                                                                  *
 *                                                                            *
 ******************************************************************************/
void	ltrim_spaces(char *c)
{
    int i;
    /* Number of left spaces */
    int spaces=0;

    for(i=0; c[i]!=0; i++)
    {
        if( c[i] == ' ')
        {
            spaces++;
        }
        else	break;
    }
    for(i=0; c[i+spaces]!=0; i++)
    {
        c[i]=c[i+spaces];
    }

    c[strlen(c)-spaces]=0;
}

/******************************************************************************
 *                                                                            *
 * Function: lrtrim_spaces                                                    *
 *                                                                            *
 * Purpose: delete all left and right spaces for the string                   *
 *                                                                            *
 * Parameters: c - string to trim spaces                                      *
 *                                                                            *
 * Return value: string without left and right spaces                         *
 *                                                                            *
 * Author: Alexei Vladishev                                                   *
 *                                                                            *
 * Comments:                                                                  *
 *                                                                            *
 ******************************************************************************/
void	lrtrim_spaces(char *c)
{
    ltrim_spaces(c);
    rtrim_spaces(c);
}

//得到应用名称
std::string getExeFileName()
{
    char procName[256];
    sprintf(procName, "/proc/%d/exe", getpid());

    std::string re;
    re.resize(256);
    if (readlink(procName, &re[0], re.length())<0)
    {
        perror("read link error\n");
        return "";
    }

    size_t n = re.rfind("/");
    if (n == re.npos)
        return "";
    else
        return re.substr(n+1);
}

//得到应用路径，不带os.sep
std::string getExeFilePath()
{
    char procName[256];
    sprintf(procName, "/proc/%d/exe", getpid());

    std::string re;
    re.resize(256);
    if (readlink(procName, &re[0], re.length())<0)
    {
        perror("read link error\n");
        return "";
    }

    std::string::size_type n = re.rfind("/");
    if (n == re.npos)
        return "";
    else
        return re.substr(0, n);
}


int createPIDFile(const char* fileName)
{
    char buffer[256];
    char value[64];
    memset(buffer, 0, sizeof(buffer));
    memset(value, 0, sizeof(value));

    if (fileName==NULL || fileName[0]==0)
    {
        std::string sf = getExeFileName();
        strcpy(buffer, sf.c_str());
        strcat(buffer, ".pid");
        fileName=buffer;
    }

    int fd = open(fileName, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
    if (fd<0)
    {
        printf("open file %s error\n", fileName);
        return -1;
    }

    sprintf(value, "%u\n", getpid());

    write(fd, value, sizeof(value));

    close(fd);

    return 0;
}

int dropPIDFile(const char *fileName)
{
    char buffer[256];

    if (fileName==NULL || fileName[0]==0)
    {
        std::string sf = getExeFileName();
        strcpy(buffer, sf.c_str());
        strcat(buffer, ".pid");
        fileName=buffer;
    }

    return remove(fileName);
}

int splitString(std::string &str, const char* pDelimiter, std::vector<std::string> & strVec)
{
    int pos = 0;
    int offset = 0;
    while( (pos=str.find(pDelimiter, offset))>0 )
    {
        strVec.push_back(str.substr(offset, pos-offset));
        offset = pos + 1;
    }
    if (offset<str.length())
        strVec.push_back(str.substr(offset));
    return 0;
}

void toLower(std::string &s)
{
    std::string::iterator iter = s.begin();
    for (; iter!=s.end(); ++iter)
    {
        *iter = tolower(*iter);
    }
}

void toUpper(std::string &s)
{
    std::string::iterator iter = s.begin();
    for (; iter!=s.end(); ++iter)
    {
        *iter = toupper(*iter);
    }
}

}
}
