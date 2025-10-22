// Copyright (c) 2025 Inan Evin
#pragma once

#include "common/size_definitions.hpp"
#include "data/string.hpp"
#include "math/color.hpp"

namespace SFG
{
	class ostream;
	class istream;

	enum class light_raw_type : uint8
	{
		point,
		spot,
		sun
	};

	struct light_raw
	{
		color		   base_color = color::white;
		float		   intensity  = 0.0f;
		float		   range	  = 0.0;
		float		   inner_cone = 0.0f;
		float		   outer_cone = 0.0f;
		light_raw_type type		  = {};

		void serialize(ostream& stream) const;
		void deserialize(istream& stream);
	};

}
