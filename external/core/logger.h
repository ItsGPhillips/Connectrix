//
// Created by George on 10/24/2021.
//

#pragma warning(push, 0)
#include <spdlog/fmt/ostr.h>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/fmt.h>
#pragma warning(pop)

#include "defs.h"

#pragma once
namespace core
{
	class Logger
	{
	public:
		static void init ();
		static std::shared_ptr<spdlog::logger>& get ()
		{
			if (S_LOGGER == nullptr)
			{
				Logger::init ();
			}
			return S_LOGGER;
		}

	private:
		Logger () = default;
		static Rc<spdlog::logger> S_LOGGER;
	};
	void launchConsole ();
#if NDEBUG
#define LOG_DEBUG(...) FORCE_MACRO_SEMICOLON()
#define LOG_TRACE(...) FORCE_MACRO_SEMICOLON()
#define LOG_INFO(...) FORCE_MACRO_SEMICOLON()
#define LOG_WARN(...) FORCE_MACRO_SEMICOLON()
#define LOG_ERROR(...) FORCE_MACRO_SEMICOLON()
#define LOG_CRITICAL(...) FORCE_MACRO_SEMICOLON()
#else
#define LOG_DEBUG(...) FORCE_MACRO_SEMICOLON (::core::Logger::get ()->debug (__VA_ARGS__);)
#define LOG_TRACE(...) FORCE_MACRO_SEMICOLON (::core::Logger::get ()->trace (__VA_ARGS__);)
#define LOG_INFO(...) FORCE_MACRO_SEMICOLON (::core::Logger::get ()->info (__VA_ARGS__);)
#define LOG_WARN(...) FORCE_MACRO_SEMICOLON (::core::Logger::get ()->warn (__VA_ARGS__);)
#define LOG_ERROR(...) FORCE_MACRO_SEMICOLON (::core::Logger::get ()->error (__VA_ARGS__);)
#define LOG_CRITICAL(...) FORCE_MACRO_SEMICOLON (::core::Logger::get ()->critical (__VA_ARGS__);)
#endif
} // namespace core
