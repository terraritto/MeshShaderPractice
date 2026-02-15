#pragma once
#include <cstdint>
#include <Windows.h>

enum class LOG_LEVEL
{
	LOG_VERBOSE = 0,
	LOG_INFO,
	LOG_DEBUG,
	LOG_WARNING,
	LOG_ERROR
};

class ConsoleColor
{
public:
	explicit ConsoleColor(LOG_LEVEL level);
	~ConsoleColor();

private:
	CONSOLE_SCREEN_BUFFER_INFO m_info = {};
};

class ConsoleLogger
{
public:
	static ConsoleLogger& Instance();

	// dump log
	void WriteA(const LOG_LEVEL level, const char* format, ...);
	void WriteW(const LOG_LEVEL level, const wchar_t* format, ...);

	// filter
	void SetFilter(const LOG_LEVEL level);
	const LOG_LEVEL GetFilter() const;

private:
	ConsoleLogger();
	ConsoleLogger(const ConsoleLogger&) = delete;
	ConsoleLogger& operator=(const ConsoleLogger&) = delete;

private:
	static ConsoleLogger m_logger;
	LOG_LEVEL m_filter;
};


#if defined(DEBUG) || defined(_DEBUG)
#define DLOGA( fmt, ... )      ConsoleLogger::Instance().WriteA( LOG_LEVEL::LOG_DEBUG, "[File: %s, Line: %d] " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__ )
#else
#define DLOGA( fmt, ... )      ((void)0)
#endif

#if defined(DEBUG) || defined(_DEBUG)
#define DLOGW( fmt, ... )      ConsoleLogger::Instance().WriteW( LOG_LEVEL::LOG_DEBUG, L"[File: %s, Line: %d] " Lfmt L"\n", L__FILE__, __LINE__, ##__VA_ARGS__ )
#else
#define DLOGW( fmt, ... )      ((void)0)
#endif

#define VLOGA( fmt, ... )   ConsoleLogger::Instance().WriteA( LOG_LEVEL::LOG_VERBOSE, fmt "\n", ##__VA_ARGS__ )
#define VLOGW( fmt, ... )   ConsoleLogger::Instance().WriteW( LOG_LEVEL::LOG_VERBOSE, Lfmt L"\n", ##__VA_ARGS__ )
#define ILOGA( fmt, ... )   ConsoleLogger::Instance().WriteA( LOG_LEVEL::LOG_INFO, fmt "\n", ##__VA_ARGS__ )
#define ILOGW( fmt, ... )   ConsoleLogger::Instance().WriteW( LOG_LEVEL::LOG_INFO, Lfmt L"\n", ##__VA_ARGS__ );
#define WLOGA( fmt, ... )   ConsoleLogger::Instance().WriteA( LOG_LEVEL::LOG_WARNING, fmt "\n", ##__VA_ARGS__ )
#define WLOGW( fmt, ... )   ConsoleLogger::Instance().WriteW( LOG_LEVEL::LOG_WARNING, Lfmt L"\n", ##__VA_ARGS__ )
#define ELOGA( fmt, ... )   ConsoleLogger::Instance().WriteA( LOG_LEVEL::LOG_ERROR, "[File: %s, Line: %d] " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__ )
#define ELOGW( fmt, ... )   ConsoleLogger::Instance().WriteW( LOG_LEVEL::LOG_ERROR, L"[File: %s, Line: %d] " Lfmt L"\n", L__FILE__, __LINE__, ##__VA_ARGS__ )
