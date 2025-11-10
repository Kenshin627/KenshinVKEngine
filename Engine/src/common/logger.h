#pragma once
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <memory>

class Log
{
public:
	static void Init();
	static std::shared_ptr<spdlog::logger> GetCoreLogger() { return mCoreLogger; }
private:
	static std::shared_ptr<spdlog::logger> mCoreLogger;
};

#define KS_CORE_TRACE(...) ::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define KS_CORE_INFO(...)  ::Log::GetCoreLogger()->info(__VA_ARGS__)
#define KS_CORE_WARN(...)  ::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define KS_CORE_ERROR(...) ::Log::GetCoreLogger()->error(__VA_ARGS__)
#define KS_CORE_FATAL(...) ::Log::GetCoreLogger()->critical(__VA_ARGS__)


