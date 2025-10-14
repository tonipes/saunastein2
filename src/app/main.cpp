// Copyright (c) 2025 Inan Evin

#define WIN32_LEAN_AND_MEAN
#include "Windows.h"

#include "game.hpp"
#include "io/log.hpp"
#include "memory/memory_tracer.hpp"
#include "platform/process.hpp"

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR pCmdLine, _In_ int nCmdShow)
{
	if (AllocConsole() == FALSE)
	{
		SFG_ERR("Failed allocating console!");
	}
	SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
	SetProcessPriorityBoost(GetCurrentProcess(), FALSE);

	// Avoid over-constraining scheduling which can cause hitches.
	// Do not pin to a single CPU and avoid realtime priority.
	if (!SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS))
	{
		DWORD dwError = GetLastError();
		SFG_ERR("Failed setting process priority: {0}", dwError);
	}

	PUSH_MEMORY_CATEGORY("General");

	{
		SFG::game_app app;

		try
		{
			app.init(SFG::vector2ui16(1920, 1080));
		}
		catch (std::exception e)
		{
			MessageBox(nullptr, e.what(), "Error", MB_OK | MB_ICONERROR);
			FreeConsole();
			return 0;
		}

		app.tick();
		app.uninit();
	}

	POP_MEMORY_CATEGORY();

	FreeConsole();

	return 0;
}
