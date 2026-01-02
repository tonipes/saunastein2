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

#include "color.hpp"
#include "math.hpp"
#include "data/istream.hpp"
#include "data/ostream.hpp"

#ifdef SFG_TOOLMODE
#include <vendor/nhlohmann/json.hpp>
using json = nlohmann::json;
#endif

namespace SFG
{
	color color::red	= color(1, 0, 0, 1);
	color color::green	= color(0, 1, 0);
	color color::blue	= color(0, 0, 1);
	color color::cyan	= color(0, 1, 1);
	color color::yellow = color(1, 1, 0);
	color color::black	= color(0, 0, 0);
	color color::white	= color(1, 1, 1);
	color color::purple = color(1, 0, 1);
	color color::maroon = color(0.5f, 0, 0);
	color color::beige	= color(0.96f, 0.96f, 0.862f);
	color color::brown	= color(0.647f, 0.164f, 0.164f);
	color color::gray	= color(0.5f, 0.5f, 0.5f);

	color color::linear_to_srgb()
	{
		auto convert = [](float value) {
			if (value <= 0.0031308f)
			{
				return value * 12.92f;
			}
			else
			{
				return 1.055f * math::pow(value, 1.0f / 2.4f) - 0.055f;
			}
		};

		return color(convert(x), convert(y), convert(z), convert(w));
	}

	color color::srgb_to_linear()
	{
		auto convert = [](float value) {
			if (value <= 0.04045f)
			{
				return value / 12.92f;
			}
			else
			{
				return math::pow((value + 0.055f) / 1.055f, 2.4f);
			}
		};

		return color(convert(x), convert(y), convert(z), convert(w));
	}

	color color::from255(float r, float g, float b, float a)
	{
		return color(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f);
	}

	void color::round()
	{
		x = math::round(x);
		y = math::round(y);
		z = math::round(z);
		w = math::round(w);
	}

	void color::serialize(ostream& stream) const
	{
		stream << x << y << z << w;
	}

	void color::deserialize(istream& stream)
	{
		stream >> x >> y >> z >> w;
	}

	vector4 color::to_vector() const
	{
		return vector4(x, y, z, w);
	}

#ifdef SFG_TOOLMODE

	void to_json(nlohmann::json& j, const color& c)
	{
		j["x"] = c.x;
		j["y"] = c.y;
		j["z"] = c.z;
		j["w"] = c.w;
	}

	void from_json(const nlohmann::json& j, color& c)
	{
		c.x = j.value<float>("x", 0.0f);
		c.y = j.value<float>("y", 0.0f);
		c.z = j.value<float>("z", 0.0f);
		c.w = j.value<float>("w", 0.0f);
	}
#endif

} // namespace SFG
