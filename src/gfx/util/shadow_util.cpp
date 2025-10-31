// Copyright (c) 2025 Inan Evin
#include "shadow_util.hpp"
#include "common/size_definitions.hpp"
#include "math/matrix4x4.hpp"
#include "math/vector3.hpp"
#include "math/vector2ui16.hpp"
#include "math/vector2.hpp"
#include "math/math.hpp"

namespace SFG
{
	void shadow_util::get_world_space_ndc(const matrix4x4& inv_view_proj, static_vector<vector4, 8>& out_world_space, vector3& out_center)
	{
		for (uint8 x = 0; x < 2; x++)
		{
			for (uint8 y = 0; y < 2; y++)
			{
				for (uint8 z = 0; z < 2; z++)
				{
					const vector4 v	 = inv_view_proj * vector4(2.0f * x - 1.0f, 2.0f * y - 1.0f, z, 1.0f);
					const vector4 ws = v / v.w;
					out_world_space.push_back(ws);
				}
			}
		}

		out_center = vector3::zero;
		for (const vector4& v : out_world_space)
			out_center += vector3(v.x, v.y, v.z);
		out_center /= static_cast<float>(out_world_space.size());
	}

	void shadow_util::get_lightspace_projection(matrix4x4& out_proj, const matrix4x4& light_view, const static_vector<vector4, 8>& world_space_ndc, const vector2ui16& resolution, vector2& out_texel_size)
	{
		{

			float min_x = std::numeric_limits<float>::max();
			float max_x = std::numeric_limits<float>::lowest();
			float min_y = std::numeric_limits<float>::max();
			float max_y = std::numeric_limits<float>::lowest();
			float min_z = std::numeric_limits<float>::max();
			float max_z = std::numeric_limits<float>::lowest();
			for (const auto& v : world_space_ndc)
			{
				const auto trf = light_view * v;
				min_x		   = math::min(min_x, trf.x);
				max_x		   = math::max(max_x, trf.x);
				min_y		   = math::min(min_y, trf.y);
				max_y		   = math::max(max_y, trf.y);
				min_z		   = math::min(min_z, trf.z);
				max_z		   = math::max(max_z, trf.z);
			}

			// Current bounds from your light-space frustum fit:
			// float orthoWidth  = max_x - min_x;
			// float orthoHeight = max_y - min_y;
			//
			// // Texel size in light space:
			// float texelX = orthoWidth / static_cast<float>(resolution.x);
			// float texelY = orthoHeight / static_cast<float>(resolution.y);
			//
			// // Center before snapping
			// vector2 center = {0.5f * (min_x + max_x), 0.5f * (min_y + max_y)};
			//
			// // Snap center to texel grid
			// center.x = floor(center.x / texelX + 0.5f) * texelX;
			// center.y = floor(center.y / texelY + 0.5f) * texelY;
			//
			// // Rebuild min/max using snapped center (KEEP SIZE THE SAME)
			// min_x = center.x - 0.5f * orthoWidth;
			// max_x = center.x + 0.5f * orthoWidth;
			// min_y = center.y - 0.5f * orthoHeight;
			// max_y = center.y + 0.5f * orthoHeight;

			constexpr float zMult = 10.0f;

			if (min_z < 0)
			{
				min_z *= zMult;
			}
			else
			{
				min_z /= zMult;
			}
			if (max_z < 0)
			{
				max_z /= zMult;
			}
			else
			{
				max_z *= zMult;
			}

			// Near/far as you already compute:
			float near_dist = -max_z;
			float far_dist	= -min_z;

			// Build the same D3D-style ortho you already have
			out_proj = matrix4x4::ortho(min_x, max_x, max_y, min_y, near_dist, far_dist);
			out_texel_size = vector2(max_x - min_x / static_cast<float>(resolution.x), max_y - min_y / static_cast<float>(resolution.y));
		}

		//
	}
}