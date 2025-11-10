#include "logger.h"

std::shared_ptr<spdlog::logger> Log::mCoreLogger;

void Log::Init()
{
	spdlog::set_pattern("%^[%T] %n: %v%$");
	spdlog::set_level(spdlog::level::trace);
	mCoreLogger = spdlog::stdout_color_mt("CORE");
}