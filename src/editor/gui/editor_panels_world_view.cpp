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
#include "editor_panels_world_view.hpp"
#include "math/vector2ui16.hpp"
#include "imgui.h"
#include "editor/editor.hpp"
#include "gfx/backend/backend.hpp"

namespace SFG
{
	static vector2ui16 _wv_last = {};
	static vector2ui16 _wv_pending = {};
	static bool _wv_has_pending = false;
	static bool _wv_has_committed = false;

	void editor_panels_world_view::init()
	{
	}

	void editor_panels_world_view::uninit()
	{
	}

	void editor_panels_world_view::draw(const vector2ui16& window_size)
	{
		static bool open = true;
		if (ImGui::Begin("World View", &open))
		{
			ImVec2 avail = ImGui::GetContentRegionAvail();
			ImGui::BeginChild("WorldViewChild", avail, false);
			ImVec2 win_sz = ImGui::GetWindowSize();
			vector2ui16 cur(static_cast<uint16>(win_sz.x), static_cast<uint16>(win_sz.y));
			if ((_wv_last.x != cur.x || _wv_last.y != cur.y))
			{
				_wv_pending = cur;
				_wv_has_pending = true;
			}
			if (_wv_has_pending && !ImGui::IsAnyMouseDown())
			{
				_wv_last = _wv_pending;
				_wv_has_pending = false;
				_wv_has_committed = true;
			}
			if (_wv_last.x == 0 || _wv_last.y == 0)
			{
				_wv_last = cur;
				_wv_has_committed = true;
			}
			uint32 idx = editor::get().get_world_rt_gpu_index();
			if (idx != 0 && avail.x > 1.0f && avail.y > 1.0f)
			{
				ImU64 handle = gfx_backend::get()->get_srv_gpu_handle_from_index(idx);
				ImGui::Image(ImTextureRef(handle), avail);
			}
			ImGui::EndChild();
		}
		ImGui::End();
	}

	bool editor_panels_world_view::consume_committed_size(vector2ui16& out_size)
	{
		if (!_wv_has_committed)
			return false;
		_wv_has_committed = false;
		out_size = _wv_last;
		return true;
	}
}
