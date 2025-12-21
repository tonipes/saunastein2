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

namespace SFG
{
	class easing
	{
	public:
		static float smooth_damp(float current, float target, float* currentVelocity, float smoothTime, float maxSpeed, float deltaTime);
		static float lerp(float val1, float val2, float amt);
		static float cubic_lerp(float val1, float val2, float amt);
		static float cubic_interp(float val0, float val1, float val2, float val3, float amt);
		static float cubic_interp_tangents(float val1, float tan1, float val2, float tan2, float amt);
		static float bilerp(float val00, float val10, float val01, float val11, float amtX, float amtY);
		static float step(float edge, float x);
		static float ease_in(float start, float end, float alpha);
		static float ease_out(float start, float end, float alpha);
		static float ease_in_out(float start, float end, float alpha);
		static float cubic(float start, float end, float alpha);
		static float exponential(float start, float end, float alpha);
		static float bounce(float start, float end, float alpha);
		static float sinusodial(float start, float end, float alpha);
	};
} // namespace SFG
