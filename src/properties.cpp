#include <cstdio>
#include <cstring>
#include <cstdlib>
#include "properties.h"
#include "utils.h"
#include "logger.h"


using namespace std;

namespace myan{
namespace utils{

Properties::Properties()
{
    //ctor
}

Properties::Properties(std::string fileName)
{
    this->_fileName = fileName;
}

Properties::~Properties()
{
    std::cout << "delete properties" << std::endl;
}

void Properties::load(std::string fileName)
{
    this->_fileName = fileName;

    char
        line[MAX_STR_SIZE],
        *value;
    int lineno=0;

#define CFG_LTRIM_CHARS "\t "
#define CFG_RTRIM_CHARS CFG_LTRIM_CHARS "\r\n\0";

    FILE *fp = fopen(this->_fileName.c_str(), "r");
    if (fp<0)
    {
        printf("open properties file %s error", fileName.c_str());
        exit(1);
    }

    for (lineno=0; fgets(line, MAX_STR_SIZE, fp)!=NULL; ++lineno)
    {
        if (line[0]=='#') continue;
        if (strlen(line)<3) continue;

        //get section name
        char *pSection;
        rtrim_chars(line, "\r\n\0");
        if (line[0]=='[' && line[strlen(line) - 1]==']')
        {
            pSection = line;
            //TODO section 没有做任何处理，正常应该保存section到map中
            continue;
        }

        value = strstr(line, "=");
        *value='\0'; //将‘=’复制为\0
        value++;

        char *pkey = new char[value-line+1];
        memset(pkey, 0 , sizeof(pkey));
        char *p = line;
        int len = value-p;
        for(int i=0; i<len; i++)
            pkey[i] = *p++;

        ltrim_chars(value, "\r\n\0");

        this->_contentMap.insert( std::make_pair(std::string(pkey),std::string(value)) );
        delete [] pkey;
    }

    fclose(fp);
}

std::string Properties::getString(std::string skey)
{
    return this->_contentMap[skey];
}

int Properties::getInt(std::string skey)
{
    std::string value = this->getString(skey);
    return atoi(&value[0]);
}

bool Properties::getBool(std::string skey)
{
    std::string value = this->getString(skey);
    for (std::string::iterator iter=value.begin(); iter!=value.end(); ++iter)
        *iter = tolower(*iter);
    if(value=="true") return true;
    else return false;
}

void Properties::setFileName(std::string fileName)
{
    this->_fileName = fileName;
}

class PropertiesSingleton
{
    public:
        PropertiesSingleton()
        {
            _proper=NULL;
        }
        ~PropertiesSingleton()
        {
            delete _proper;
        }
        Properties *getProperties()
        {
            boost::mutex::scoped_lock scope(_mutex);
            if (_proper==NULL)
                _proper = new Properties();
            return _proper;
        }
    private:
        Properties *_proper;
        boost::mutex _mutex;
};

static PropertiesSingleton propertiesSingleton;

Properties &Properties::getProperties()
{
    return *(propertiesSingleton.getProperties());
}


}
}

