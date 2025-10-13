// Copyright (c) 2025 Inan Evin

#pragma once
#include "common/size_definitions.hpp"
#include "data/static_vector.hpp"
#include "math/frustum.hpp"
#include "math/matrix4x4.hpp"

namespace SFG
{
	class world;
	struct vector2ui16;

	struct view
	{
		matrix4x4 view_matrix	   = matrix4x4::identity;
		matrix4x4 proj_matrix	   = matrix4x4::identity;
		matrix4x4 view_proj_matrix = matrix4x4::identity;
		frustum	  view_frustum	   = {};
	};

#define MAX_VIEWS 24

	enum view_index : uint8
	{
		view_index_main_camera = 0,
		view_index_directional,
		view_index_light0,
		view_index_light1,
		view_index_light2,
		view_index_light3,
		view_index_light4,
		view_index_light5,
		view_index_light6,
		view_index_light7,
		view_index_max,
	};

	class view_manager
	{
	public:
		void reset();
		void generate_views(world& w, const vector2ui16& res, double interpolation);

		bool visibility_test_any_view(const aabb& box);
		bool visibility_test_view(const aabb& box, uint8 index);

		inline const static_vector<view, view_index::view_index_max>& get_views() const
		{
			return _views;
		}

		inline view& get_view(uint8 index)
		{
			return _views[index];
		}

	private:
		static_vector<view, view_index::view_index_max> _views;
		uint32											_view_count = 0;
	};
}
