// Copyright (c) 2025 Inan Evin

#include "app.hpp"
#include "memory/memory_tracer.hpp"
#include "platform/process.hpp"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR pCmdLine, _In_ int nCmdShow)
{
	if (AllocConsole() == FALSE)
	{
	}

	SFG::process::init();

	PUSH_MEMORY_CATEGORY("General");

	{
		SFG::app app;

		try
		{
			
		}
		catch (std::exception e)
		{
			MessageBox(nullptr, e.what(), "Error", MB_OK | MB_ICONERROR);
			FreeConsole();
			return 0;
		}

		const SFG::app::init_status status = app.init(SFG::vector2ui16(1920, 1080));
		if (status != SFG::app::init_status::ok)
		{
			if (status == SFG::app::init_status::working_directory_dont_exist)
				SFG::process::message_box("Toolmode requires a valid working directory!");
			else if (status == SFG::app::init_status::renderer_failed)
				SFG::process::message_box("Renderer failed initializing!");
			else if (status == SFG::app::init_status::backend_failed)
				SFG::process::message_box("Gfx backend failed initializing!");
			else if (status == SFG::app::init_status::window_failed)
				SFG::process::message_box("Main window failed initializing!");
			else if (status == SFG::app::init_status::engine_shaders_failed)
				SFG::process::message_box("Failed loading engine shaders!");

			SFG::process::uninit();
			POP_MEMORY_CATEGORY();
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
