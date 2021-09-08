#ifndef __TMPLOG_H__
#define __TMPLOG_H__

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <string>
#include <assert.h>

#define __W(str)    L##str
#define _W(str)     __W(str)

typedef enum LOGGER_LEVEL
{
    TrackLevel      = 0,
    InLevel         = 1,
    OutLevel        = 2,
    InfoLevel       = 3,
    WarningLevel    = 4,
    ErrorLevel      = 5,
    FatalLevel      = 6,
    DisableLevel    = 99,
} ENM_LOGGER_LEVEL;

class Log
{
public:
    Log(const std::wstring& strFilePath, const std::wstring& strFunctionName, int iLine);
    ~Log();
public:
    static void SetLogPath(const std::wstring& strLogPath);
    static void Write(ENM_LOGGER_LEVEL logLevel,
        const std::wstring& strFilePath,
        const std::wstring& strFunctionName,
        int iLine,
        LPCWSTR pszFormat, ...);
public:
    static FILE* _CreateFile();
    static std::wstring _GetModuleName();
    static std::wstring _GetShortFuncName(const std::wstring& strFunctionName);
    static std::wstring _GetDateTimeString();
    static std::wstring _GetDWORString(DWORD dwVal);
    static std::wstring _GetLevel(ENM_LOGGER_LEVEL logLevel);
private:
    const std::wstring      m_strFilePath;
    const std::wstring      m_strFunctionName;
    int                     m_iLine;
};

#define LTRACE(formatstr, ...)      Log::Write(TrackLevel,    _W(__FILE__), _W(__FUNCTION__), __LINE__, formatstr, ##__VA_ARGS__)
#define LINFO(formatstr, ...)       Log::Write(InfoLevel,     _W(__FILE__), _W(__FUNCTION__), __LINE__, formatstr, ##__VA_ARGS__)

#define LWARNING(formatstr, ...)                                                                            \
            do                                                                                                  \
            {                                                                                                   \
                Log::Write(WarningLevel, _W(__FILE__), _W(__FUNCTION__), __LINE__, formatstr, ##__VA_ARGS__);   \
            } while (false)

#define LERROR(formatstr, ...)                                                                              \
            do                                                                                                  \
            {                                                                                                   \
                Log::Write(ErrorLevel, _W(__FILE__), _W(__FUNCTION__), __LINE__, formatstr, ##__VA_ARGS__);     \
            } while (false)

#define LFATAL(formatstr, ...)                                                                              \
            do                                                                                                  \
            {                                                                                                   \
                Log::Write(FatalLevel, _W(__FILE__), _W(__FUNCTION__), __LINE__, formatstr, ##__VA_ARGS__);     \
            } while (false)

#define LOGOUT(level, formatstr, ...)                                                                       \
            do                                                                                                  \
            {                                                                                                   \
                Log::Write(level, _W(__FILE__), _W(__FUNCTION__), __LINE__, formatstr, ##__VA_ARGS__);           \
                if (WarningLevel == level || ErrorLevel == level || FatalLevel == level)                        \
                {                                                                                               \
                }                                                                                               \
            } while (false)

#define LOGGER                      Log __tmp_logger__(_W(__FILE__), _W(__FUNCTION__), __LINE__)

#endif /* __TMPLOG_H__ */
