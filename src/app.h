#ifndef APPLICATION_H
#define APPLICATION_H

#include "core.h"
#include "properties.h"

class Application
{
public:
    Application();
    virtual ~Application();

    int main(int argc, char **argv);

    virtual void usage(int argc, char **argv);
    virtual void welcome() = 0;
    virtual void run() = 0;

protected:
    struct AppArgs
    {
        bool isDaemon;
        std::string pidfile;
        std::string confFile;
        std::string workDir;
        std::string startOpt;

        AppArgs()
        {
            isDaemon = false;
            startOpt = "start";
        }
    };

    AppArgs appArgs;

private:
    void parseArgs(int argc, char **argv);
    void init();

    int readPid();
    void writePid();
    void checkPidfile();
    void removePidfile();
    void killProcess();
};

#endif // APPLICATION_H
