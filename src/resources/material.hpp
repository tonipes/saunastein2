// Copyright (c) 2025 Inan Evin

#pragma once
#include "data/bitmask.hpp"
#include "data/ostream.hpp"
#include "resources/common_resources.hpp"
#include "reflection/resource_reflection.hpp"

#ifndef SFG_STRIP_DEBUG_NAMES
#include "memory/chunk_handle.hpp"
#endif

namespace SFG
{
	struct material_raw;
	class world;

	class material
	{
	public:
		~material();

		// -----------------------------------------------------------------------------
		// resource
		// -----------------------------------------------------------------------------

		void create_from_loader(const material_raw& raw, world& w, resource_handle handle, const vector<resource_handle>& samplers = {});
		void destroy(world& w, resource_handle handle);

		// -----------------------------------------------------------------------------
		// impl
		// -----------------------------------------------------------------------------

		void update_data(world& w, resource_handle handle);

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline const bitmask<uint32>& get_flags() const
		{
			return _flags;
		}

		inline ostream& get_data()
		{
			return _material_data;
		}

	private:
		ostream			_material_data = {};
		bitmask<uint32> _flags		   = 0;
#ifndef SFG_STRIP_DEBUG_NAMES
		chunk_handle32 _name;
#endif
	};

	REGISTER_RESOURCE(material, "stkmat");

}
