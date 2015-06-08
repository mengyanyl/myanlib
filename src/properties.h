#ifndef PROPERTIES_H
#define PROPERTIES_H

#include <map>
#include <string>
#include "core.h"

namespace myan{
namespace utils{

class Properties
{
    public:
        Properties();

        Properties(std::string fileName);

        virtual ~Properties();

        //加载配置文件初始化_contentMap
        void load(std::string fileName);

        std::string getString(std::string strKey);

        int getInt(std::string strKey);

        bool getBool(std::string strKey);

        static Properties& getProperties();

        void setFileName(std::string fileName);
    protected:

    private:
        //配置文件每行最大2k
        static const int MAX_STR_SIZE = 2048;

        std::string _fileName;

        //保存配置文件中的内容
        std::map<std::string, std::string> _contentMap;


};

}
}

#endif // PROPERTIES_H
