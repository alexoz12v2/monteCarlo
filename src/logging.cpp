#include "logging.h"

// pasted here code from logging.cpp because logging through multiple translation units doesn't work, as different translation units
// have different file descriptors. I don't want to log to a file. Also added inline
#include <fmt/core.h>

#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

namespace mxc
{
	auto stringFormat_v(char* dest, const char* format, va_list* va_listp)  -> int32_t
	{
		if (dest) {
			// Big, but can fit on the stack.
			char buffer[32000];
			int32_t written = vsnprintf(buffer, 32000, format, *va_listp);
			buffer[written] = 0;
			memcpy(dest, buffer, written + 1);

			return written;
		}
		return -1;
	}

    auto logOutput(LogLevel level, char const* file, int32_t const line, char const* message, ...) -> void
	{
		va_list args;
		va_start(args, message);

		// hard limit of 32k characters on log entry
		char outMessage[32000] = {'\0'};

		// format the output string
		stringFormat_v(outMessage, message, &args);
		va_end(args);
		
		// set output color according to level
		switch (level)
		{
			case LogLevel::info: break; // white info 
			case LogLevel::debug: fmt::print("\033[32m"); break; // green debug
			case LogLevel::warn: fmt::print("\033[33m"); break; // yellow warn
			case LogLevel::error: fmt::print("\033[31m"); break; // red error
			case LogLevel::trace: break; // white trace
		}
		
		// actually print the stuff
		fmt::print("{} {} [DEBUG]: ", file, line);
		fmt::print("{}", outMessage);

		// reset output color according to level
		switch (level)
		{
			case LogLevel::debug: [[fallthrough]];
			case LogLevel::warn:  [[fallthrough]];
			case LogLevel::error: [[fallthrough]];
			case LogLevel::info: [[fallthrough]];
			case LogLevel::trace: fmt::print("\033[37m\n");
		}
	}
}
