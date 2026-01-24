/*
This file is a part of stakeforge_engine: https://github.com/inanevin/stakeforge
Copyright [2025-] Inan Evin

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice, this
      list of conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
*/

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
			else if (status == SFG::app::init_status::engine_resources_failed)
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
