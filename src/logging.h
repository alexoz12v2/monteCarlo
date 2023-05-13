#ifndef MXC_LOGGING_H
#define MXC_LOGGING_H

#include <cstdint>

#ifdef MXC_LOGLEVEL_INFO
	#define MXC_INFO(message, ...) ::mxc::logOutput(::mxc::LogLevel::info, __FILE__, __LINE__, message, ##__VA_ARGS__)
#else
	#define MXC_INFO(...)
#endif

#ifdef MXC_LOGLEVEL_DEBUG
	#define MXC_DEBUG(message, ...) ::mxc::logOutput(::mxc::LogLevel::debug, __FILE__, __LINE__, message, ##__VA_ARGS__)
#else
	#define MXC_DEBUG(...)
#endif

#ifdef MXC_LOGLEVEL_WARN
	#define MXC_WARN(message, ...) ::mxc::logOutput(::mxc::LogLevel::warn, __FILE__, __LINE__, message, ##__VA_ARGS__)
#else
	#define MXC_WARN(...)
#endif

#ifdef MXC_LOGLEVEL_ERROR
	#define MXC_ERROR(message, ...) ::mxc::logOutput(::mxc::LogLevel::error, __FILE__, __LINE__, message, ##__VA_ARGS__)
#else
	#define MXC_ERROR(...)
#endif

#ifdef MXC_LOGLEVEL_TRACE
	#define MXC_TRACE(message, ...) ::mxc::logOutput(::mxc::LogLevel::trace, __FILE__, __LINE__, message, ##__VA_ARGS__)
#else
	#define MXC_TRACE(...)
#endif

namespace mxc
{
	enum class LogLevel 
	{
		info, debug, warn, error, trace
	};

	// taken from kohi game engine https://github.com/travisvroman/kohi
	// TODO find a way to make it work with fmt::print, so that I can use its format style instead of C format specifiers
	auto stringFormat_v(char* dest, const char* format, void* va_listp) -> int32_t;
	
	auto logOutput(LogLevel level, char const* file, int32_t const line, char const* message, ...) -> void;
}
#endif // MXC_LOGGING_H
