#pragma once

enum LogType
{
    INFO,
    DEBUG,
    WARN,
    ERROR
};

inline const char *convertLogType(LogType type)
{
    switch (type)
    {
    case DEBUG:
        return "DEBUG";
    case WARN:
        return "WARN";
    case ERROR:
        return "ERROR";
    default:
        return "INFO";
    }
}

void log(string msg, LogType type)
{
    if (options.debug)
    {
        time_t now;
        time(&now);
        char buf[sizeof "0000-00-00T00:00:00Z"];
        strftime(buf, sizeof buf, "%Y-%m-%dT%H:%M:%SZ", gmtime(&now));
        cout << buf << " " << convertLogType(type) << ": " << msg << endl;
    }
}
