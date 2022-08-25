//
// Created by George on 10/24/2021.
//

#include "logger.h"

#include <cstdio>
#include <cstdlib>

#pragma warning(push, 0)
#include <spdlog/sinks/stdout_color_sinks.h>
#pragma warning(pop)

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <iostream>
#include <windows.h>

FILE* fStdOut;
FILE* fStdIn;

namespace core
{
	void BindStdHandlesToConsole () noexcept
	{
		fStdOut = new FILE;
		fStdIn = new FILE;

		// Note that there is no CONERR$ file
		HANDLE hStdout = CreateFile ("CONOUT$",
											  GENERIC_READ | GENERIC_WRITE,
											  FILE_SHARE_READ | FILE_SHARE_WRITE,
											  NULL,
											  OPEN_EXISTING,
											  FILE_ATTRIBUTE_NORMAL,
											  NULL);
		HANDLE hStdin = CreateFile ("CONIN$",
											 GENERIC_READ | GENERIC_WRITE,
											 FILE_SHARE_READ | FILE_SHARE_WRITE,
											 NULL,
											 OPEN_EXISTING,
											 FILE_ATTRIBUTE_NORMAL,
											 NULL);

		freopen_s (&fStdIn, "CONIN$", "r", stdin);
		freopen_s (&fStdOut, "CONOUT$", "w", stderr);
		freopen_s (&fStdOut, "CONOUT$", "w", stdout);

		SetStdHandle (STD_OUTPUT_HANDLE, hStdout);
		SetStdHandle (STD_ERROR_HANDLE, hStdout);
		SetStdHandle (STD_INPUT_HANDLE, hStdin);

		// Clear the error state for each of the C++ standard stream objects.
		std::wclog.clear ();
		std::clog.clear ();
		std::wcout.clear ();
		std::cout.clear ();
		std::wcerr.clear ();
		std::cerr.clear ();
		std::wcin.clear ();
		std::cin.clear ();
	}
	void launchConsole ()
	{
		if (!AllocConsole ())
		{
			FreeConsole ();
			if (!AllocConsole ())
			{
				MessageBox (NULL, "The console window was not created", NULL, MB_ICONEXCLAMATION);
			}
		}
		BindStdHandlesToConsole ();
	}

	std::shared_ptr<spdlog::logger> Logger::S_LOGGER;

	void Logger::init ()
	{
		static std::atomic<bool> consoleLaunched { false };
		static bool isLaunched = false;
		if (consoleLaunched.compare_exchange_strong (isLaunched, true))
		{
			launchConsole ();
		}

		if (S_LOGGER == nullptr)
		{
			auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt> ();
			consoleSink->set_pattern ("[%T] %^%l%$: %^%v%$");
			S_LOGGER = std::make_shared<spdlog::logger> (spdlog::logger ("LOG", consoleSink));
			spdlog::register_logger (S_LOGGER);
			S_LOGGER->set_level (spdlog::level::trace);
			S_LOGGER->flush_on (spdlog::level::trace);
		}
		else
		{
			S_LOGGER->warn ("called Logger::init() more than once");
		}
	}

} // namespace core
#else
void launchConsole () {}
#endif
