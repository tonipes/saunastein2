/*
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

#pragma once

namespace vekt
{
	class builder;
	struct font;
}

namespace SFG
{
	struct vector2ui16;

	enum class gizmo_axis
	{
		none,
		x,
		y,
		z
	};

	enum class gizmo_style
	{
		move,
		rotate,
		scale
	};

	enum class gizmo_space
	{
		global,
		local
	};

	class editor_gizmo_controls
	{
	public:
		// -----------------------------------------------------------------------------
		// lifecycle
		// -----------------------------------------------------------------------------

		void init(vekt::builder* builder);
		void draw(const vector2ui16& size);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline void set_style(gizmo_style s)
		{
			_style = s;
		}

		inline void set_active_axis(gizmo_axis ax)
		{
			_active_axis = ax;
		}

		inline void set_space(gizmo_space space)
		{
			_space = space;
		}

		inline gizmo_style get_style() const
		{
			return _style;
		}

		inline gizmo_space get_space() const
		{
			return _space;
		}

		inline gizmo_axis get_active_axis() const
		{
			return _active_axis;
		}

	private:
		vekt::builder* _builder		= nullptr;
		gizmo_style	   _style		= gizmo_style::move;
		gizmo_axis	   _active_axis = gizmo_axis::none;
		gizmo_space	   _space		= gizmo_space::global;
	};
}
