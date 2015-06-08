#include "app.h"

Application::Application()
{
    //ctor
}

Application::~Application()
{
    //dtor
}

int Application::init()
{

}

int Application::writePid()
{
    string filePath = myan::utils::getExeFilePath();
    if (filePath == "")
    {
        fprintf(stderr, "[Application:writePid] error: get filepath error");
        return -1;
    }

    string exeName = myan::utils::getExeFileName();
    if (exeName == "")
    {
        fprintf(stderr, "[Application:writePid] error: get exename error");
        return -1;
    }

    string pidFile = filePath + "/" + exeName + ".pid";
    int re = myan::utils::createPIDFile(pidFile.c_str());
    if (re < 0)
    {
        fprintf(stderr, "[Application:writePid] error: create pid file error");
        return -1;
    }

    return 0;
}

int Application::readPid()
{
    string filePath = myan::utils::getExeFilePath();
    if (filePath == "")
    {
        fprintf(stderr, "[Application:readPid] error: get filepath error");
        return -1;
    }

    string exeName = myan::utils::getExeFileName();
    if (exeName == "")
    {
        fprintf(stderr, "[Application:readPid] error: get exename error");
        return -1;
    }

    string pidFile = filePath + "/" + exeName + ".pid";

    //read pid file
    FILE *fp = fopen( pidFile.c_str(), "r");
    if ( fp==NULL )
    {
        fprintf(stderr, "[Application:readPid] error: open pid file error");
        return -1;
    }
    char buf[128];
    memset( buf, 0, sizeof(buf) );
    while ( fgets( buf, sizeof(buf), fp ) != NULL )
    {
        break;
    }

    int re = -1;
    int pid = atoi(buf);
    if ( pid>0 )
        re = pid

    return re;
}

int Application::checkPidfile()
{
    string filePath = myan::utils::getExeFilePath();
    if (filePath == "")
    {
        fprintf(stderr, "[Application:readPid] error: get filepath error");
        return -1;
    }

    string exeName = myan::utils::getExeFileName();
    if (exeName == "")
    {
        fprintf(stderr, "[Application:readPid] error: get exename error");
        return -1;
    }

    string pidFile = filePath + "/" + exeName + ".pid";

    if ( access( pidFile.c_str(), F_OK ) == 0 )
    {
        fprintf( stderr, "Fatal error!\nPidfile %s already exists!\n"
				"Kill the running process before you run this command,\n"
				"or use '-s restart' option to restart the server.\n",
				pidFile.c_str() );
        return -1;
    }
}

int Application::removePidfile()
{
     string filePath = myan::utils::getExeFilePath();
    if (filePath == "")
    {
        fprintf(stderr, "[Application:readPid] error: get filepath error");
        return -1;
    }

    string exeName = myan::utils::getExeFileName();
    if (exeName == "")
    {
        fprintf(stderr, "[Application:readPid] error: get exename error");
        return -1;
    }

    string pidFile = filePath + "/" + exeName + ".pid";

    int re = myan::utils::dropPIDFile( pidFile.c_str() );

    return re;
}

int Application::killProcess()
{
    int pid = this->readPid();

    if ( pid == -1 )
    {
		fprintf(stderr, "could not read pidfile: %s(%s)\n", "", strerror(errno));
		exit(1);
    }

    return 0;
}
