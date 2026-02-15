#include "Logger.h"
#include <cstdio>
#include <cstdarg>

#define MAX_CHAR_NUM		2048

ConsoleColor::ConsoleColor(LOG_LEVEL level)
{
	const HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
	GetConsoleScreenBufferInfo(handle, &m_info);

	WORD attribute = m_info.wAttributes;
	switch (level)
	{
	case LOG_LEVEL::LOG_VERBOSE:
		// white
		attribute = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY;
		break;

	case LOG_LEVEL::LOG_INFO:
		// green
		attribute = FOREGROUND_GREEN | FOREGROUND_INTENSITY;
		break;

	case LOG_LEVEL::LOG_DEBUG:
		// blue
		attribute = FOREGROUND_BLUE | FOREGROUND_INTENSITY;
		break;

	case LOG_LEVEL::LOG_WARNING:
		// yellow
		attribute = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
		break;

	case LOG_LEVEL::LOG_ERROR:
		attribute = FOREGROUND_RED | FOREGROUND_INTENSITY;
		break;
	}

	SetConsoleTextAttribute(handle, attribute);
}

ConsoleColor::~ConsoleColor()
{
	const HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(handle, m_info.wAttributes);
}

ConsoleLogger ConsoleLogger::m_logger;

ConsoleLogger& ConsoleLogger::Instance()
{
	return m_logger;
}

void ConsoleLogger::WriteA(const LOG_LEVEL level, const char* format, ...)
{
	if (static_cast<int>(m_filter) <= static_cast<int>(level))
	{	
		// set color
		ConsoleColor color{ level };

		// dump log
		char message[MAX_CHAR_NUM] = "\0";
		va_list argument;

		va_start(argument, message);
		vsprintf_s(message, format, argument);
		va_end(argument);

		fprintf((level == LOG_LEVEL::LOG_ERROR ? stderr : stdout), "%s", message);

		OutputDebugStringA(message);
	}
}

void ConsoleLogger::WriteW(const LOG_LEVEL level, const wchar_t* format, ...)
{
	if (static_cast<int>(m_filter) <= static_cast<int>(level))
	{
		// set color
		ConsoleColor color{ level };

		// dump log
		wchar_t message[MAX_CHAR_NUM] = L"\0";
		va_list argument;

		va_start(argument, message);
		vswprintf_s(message, format, argument);
		va_end(argument);

		fwprintf_s((level == LOG_LEVEL::LOG_ERROR ? stderr : stdout), L"%s", message);

		OutputDebugStringW(message);
	}
}

void ConsoleLogger::SetFilter(const LOG_LEVEL level)
{
	m_filter = level;
}

const LOG_LEVEL ConsoleLogger::GetFilter() const
{
	return m_filter;
}

ConsoleLogger::ConsoleLogger()
	: m_filter(LOG_LEVEL::LOG_VERBOSE)
{
}

#undef MAX_CHAR_NUM
