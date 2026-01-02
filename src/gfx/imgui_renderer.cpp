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

#include "imgui_renderer.hpp"
#include "gfx/backend/backend.hpp"
#include "platform/window.hpp"
#include "io/log.hpp"

#include "imgui.h"
#include <vendor/imgui/backends/imgui_impl_win32.h>
#include <vendor/imgui/backends/imgui_impl_dx12.h>
#include <vendor/imgui/misc/imgui_threaded_rendering.h>

namespace SFG
{
	void imgui_renderer::init(window& w)
	{
		ImGui_ImplWin32_EnableDpiAwareness();
		float main_scale = ImGui_ImplWin32_GetDpiScaleForMonitor(::MonitorFromPoint(POINT{0, 0}, MONITOR_DEFAULTTOPRIMARY));

		// Setup Dear ImGui context
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();

		ImGuiIO& io = ImGui::GetIO();
		(void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;	  // IF using Docking Branch

		// Fonts
#ifdef SFG_TOOLMODE
		{
			// const std::string p = SFG_ROOT_DIRECTORY + std::string("assets/engine/fonts/VT323-Regular.ttf");
			// ImFontConfig cfg;
			// cfg.SizePixels = 18.0f * main_scale;
			// ImFont* f = io.Fonts->AddFontFromFileTTF(p.c_str(), cfg.SizePixels, &cfg);
			// io.FontDefault = f;
		}
#endif

		// Setup Dear ImGui style
		ImGui::StyleColorsDark();
		// ImGui::StyleColorsLight();

		// Setup scaling
		ImGuiStyle& style = ImGui::GetStyle();
		style.ScaleAllSizes(main_scale);	// Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
		style.FontScaleDpi	  = main_scale; // Set initial font scale. (using io.ConfigDpiScaleFonts=true makes this unnecessary. We leave both here for documentation purpose)
		style.FontSizeBase	  = 12;
		style.TabRounding	  = 2;
		style.GrabMinSize	  = 12;
		style.GrabRounding	  = 2;
		style.IndentSpacing	  = 14;
		style.ChildRounding	  = 4;
		style.ChildBorderSize = 2;
		style.FrameBorderSize = 1.0f;

		ImVec4* colors							   = style.Colors;
		colors[ImGuiCol_Text]					   = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
		colors[ImGuiCol_TextDisabled]			   = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
		colors[ImGuiCol_WindowBg]				   = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
		colors[ImGuiCol_ChildBg]				   = ImVec4(0.01f, 0.01f, 0.01f, 1.00f);
		colors[ImGuiCol_PopupBg]				   = ImVec4(0.00f, 0.00f, 0.00f, 0.98f);
		colors[ImGuiCol_Border]					   = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
		colors[ImGuiCol_BorderShadow]			   = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_FrameBg]				   = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
		colors[ImGuiCol_FrameBgHovered]			   = ImVec4(0.51f, 0.03f, 0.15f, 0.67f);
		colors[ImGuiCol_FrameBgActive]			   = ImVec4(0.93f, 0.05f, 0.26f, 0.67f);
		colors[ImGuiCol_TitleBg]				   = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
		colors[ImGuiCol_TitleBgActive]			   = ImVec4(0.24f, 0.00f, 0.06f, 0.96f);
		colors[ImGuiCol_TitleBgCollapsed]		   = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
		colors[ImGuiCol_MenuBarBg]				   = ImVec4(0.01f, 0.01f, 0.01f, 1.00f);
		colors[ImGuiCol_ScrollbarBg]			   = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
		colors[ImGuiCol_ScrollbarGrab]			   = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
		colors[ImGuiCol_ScrollbarGrabHovered]	   = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
		colors[ImGuiCol_ScrollbarGrabActive]	   = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
		colors[ImGuiCol_CheckMark]				   = ImVec4(0.84f, 0.04f, 0.24f, 0.78f);
		colors[ImGuiCol_SliderGrab]				   = ImVec4(0.84f, 0.04f, 0.24f, 0.59f);
		colors[ImGuiCol_SliderGrabActive]		   = ImVec4(0.84f, 0.04f, 0.24f, 0.88f);
		colors[ImGuiCol_Button]					   = ImVec4(0.02f, 0.02f, 0.02f, 1.00f);
		colors[ImGuiCol_ButtonHovered]			   = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
		colors[ImGuiCol_ButtonActive]			   = ImVec4(0.01f, 0.01f, 0.01f, 1.00f);
		colors[ImGuiCol_Header]					   = ImVec4(0.84f, 0.04f, 0.24f, 0.20f);
		colors[ImGuiCol_HeaderHovered]			   = ImVec4(0.84f, 0.04f, 0.24f, 0.39f);
		colors[ImGuiCol_HeaderActive]			   = ImVec4(0.84f, 0.04f, 0.24f, 0.59f);
		colors[ImGuiCol_Separator]				   = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
		colors[ImGuiCol_SeparatorHovered]		   = ImVec4(0.84f, 0.04f, 0.24f, 0.71f);
		colors[ImGuiCol_SeparatorActive]		   = ImVec4(0.84f, 0.04f, 0.24f, 1.00f);
		colors[ImGuiCol_ResizeGrip]				   = ImVec4(0.84f, 0.04f, 0.24f, 0.20f);
		colors[ImGuiCol_ResizeGripHovered]		   = ImVec4(0.84f, 0.04f, 0.24f, 0.59f);
		colors[ImGuiCol_ResizeGripActive]		   = ImVec4(0.84f, 0.04f, 0.24f, 1.00f);
		colors[ImGuiCol_InputTextCursor]		   = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
		colors[ImGuiCol_TabHovered]				   = ImVec4(0.84f, 0.04f, 0.24f, 0.20f);
		colors[ImGuiCol_Tab]					   = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_TabSelected]			   = ImVec4(0.84f, 0.04f, 0.24f, 0.63f);
		colors[ImGuiCol_TabSelectedOverline]	   = ImVec4(0.84f, 0.04f, 0.24f, 0.55f);
		colors[ImGuiCol_TabDimmed]				   = ImVec4(0.07f, 0.10f, 0.15f, 0.97f);
		colors[ImGuiCol_TabDimmedSelected]		   = ImVec4(0.84f, 0.04f, 0.24f, 0.39f);
		colors[ImGuiCol_TabDimmedSelectedOverline] = ImVec4(0.50f, 0.50f, 0.50f, 0.00f);
		colors[ImGuiCol_DockingPreview]			   = ImVec4(0.84f, 0.04f, 0.24f, 0.78f);
		colors[ImGuiCol_DockingEmptyBg]			   = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
		colors[ImGuiCol_PlotLines]				   = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
		colors[ImGuiCol_PlotLinesHovered]		   = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
		colors[ImGuiCol_PlotHistogram]			   = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
		colors[ImGuiCol_PlotHistogramHovered]	   = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
		colors[ImGuiCol_TableHeaderBg]			   = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
		colors[ImGuiCol_TableBorderStrong]		   = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
		colors[ImGuiCol_TableBorderLight]		   = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
		colors[ImGuiCol_TableRowBg]				   = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_TableRowBgAlt]			   = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
		colors[ImGuiCol_TextLink]				   = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
		colors[ImGuiCol_TextSelectedBg]			   = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
		colors[ImGuiCol_TreeLines]				   = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
		colors[ImGuiCol_DragDropTarget]			   = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
		colors[ImGuiCol_DragDropTargetBg]		   = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
		colors[ImGuiCol_UnsavedMarker]			   = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
		colors[ImGuiCol_NavCursor]				   = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
		colors[ImGuiCol_NavWindowingHighlight]	   = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
		colors[ImGuiCol_NavWindowingDimBg]		   = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
		colors[ImGuiCol_ModalWindowDimBg]		   = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);

		// Setup Platform/Renderer backends
		ImGui_ImplWin32_Init(static_cast<HWND>(w.get_window_handle()));

		// Setup Platform/Renderer backends
		ImGui_ImplDX12_InitInfo init_info = {};
		init_info.Device				  = gfx_backend::get()->get_device();
		init_info.CommandQueue			  = gfx_backend::get()->get_queue_gfx_impl();
		init_info.NumFramesInFlight		  = BACK_BUFFER_COUNT;
		init_info.RTVFormat				  = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

		init_info.SrvDescriptorHeap	   = gfx_backend::get()->get_srv_heap();
		init_info.SrvDescriptorAllocFn = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_handle) { return gfx_backend::get()->alloc_srv(out_cpu_handle, out_gpu_handle); };
		init_info.SrvDescriptorFreeFn  = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle) { return gfx_backend::get()->free_srv(cpu_handle, gpu_handle); };
		ImGui_ImplDX12_Init(&init_info);

		_snapshots = new ImDrawDataSnapshot[BUFFER_SIZE];
	}

	void imgui_renderer::uninit()
	{
		delete[] _snapshots;
		_snapshots = nullptr;

		ImGui_ImplDX12_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
	}

	void imgui_renderer::new_frame()
	{
		ImGui_ImplDX12_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		static bool open = true;
		ImGui::ShowDemoWindow(&open);
	}

	void imgui_renderer::end_frame()
	{
		ImGui::Render();

		// Process texture create/update/destroy on the main thread
		ImDrawData* dd = ImGui::GetDrawData();
		if (dd->Textures)
		{
			for (ImTextureData* tex : *dd->Textures)
				if (tex->Status != ImTextureStatus_OK)
					ImGui_ImplDX12_UpdateTexture(tex);
		}

		int r = _rendered.load(std::memory_order_acquire);
		int l = _latest.load(std::memory_order_acquire);

		int next = (l + 1) % BUFFER_SIZE;
		if (next == r)
		{
			// ring full -> drop this snapshot (or spin/wait)
			return;
		}

		_snapshots[next].SnapUsingSwap(ImGui::GetDrawData(), ImGui::GetTime());
		_snapshots[next].DrawData.Textures = nullptr;
		_latest.store(next, std::memory_order_release);
	}

	void imgui_renderer::render(gfx_id cmd_buffer)
	{
		int idx = _latest.load(std::memory_order_acquire);
		if (idx < 0)
			return;
		gfx_backend* backend = gfx_backend::get();
		_rendered.store(idx, std::memory_order_release);
		ImGui_ImplDX12_RenderDrawData(&_snapshots[idx].DrawData, backend->get_gfx_cmd_list(cmd_buffer));
		_rendered.store(-1, std::memory_order_release);
	}
}
