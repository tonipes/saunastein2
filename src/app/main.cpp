// Copyright (c) 2025 Inan Evin

#define WIN32_LEAN_AND_MEAN
#include "Windows.h"

#include "game_app.hpp"
#include "io/log.hpp"
#include "memory/memory_tracer.hpp"
#include "platform/process.hpp"

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR pCmdLine, _In_ int nCmdShow)
{
	if (AllocConsole() == FALSE)
	{
		SFG_ERR("Failed allocating console!");
	}
	SFG::process::init();

	PUSH_MEMORY_CATEGORY("General");

	{
		SFG::game_app app;

		try
		{
			const SFG::game_app::init_status status = app.init(SFG::vector2ui16(1920, 1080));
			if (status != SFG::game_app::init_status::ok)
			{
				if (status == SFG::game_app::init_status::working_directory_dont_exist)
					SFG::process::message_box("Toolmode requires a valid working directory!");
				else if (status == SFG::game_app::init_status::renderer_failed)
					SFG::process::message_box("Renderer failed initializing!");
				SFG::process::uninit();
				POP_MEMORY_CATEGORY();
				FreeConsole();
				return 0;
			}
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

	SFG::process::uninit();

	return 0;
}
