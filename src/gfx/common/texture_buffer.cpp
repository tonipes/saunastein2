// Copyright (c) 2025 Inan Evin
#include "texture_buffer.hpp"
#include "data/ostream.hpp"
#include "data/istream.hpp"

namespace SFG
{

	void texture_buffer::serialize(ostream& stream, bool write_addr) const
	{
		stream << size;
		stream << bpp;
		SFG_ASSERT(pixels != nullptr);
		if (write_addr)
		{
			const uint64 addr = reinterpret_cast<uint64>(pixels);
			stream << addr;
		}
		else
		{
			const size_t sz = static_cast<size_t>(bpp * size.x * size.y);
			stream.write_raw(pixels, sz);
		}
	}
	void texture_buffer::deserialize(istream& stream, bool read_addr)
	{
		stream >> size;
		stream >> bpp;

		if (read_addr)
		{
			uint64 addr = 0;
			stream >> addr;
			pixels = reinterpret_cast<uint8*>(addr);
		}
		else
		{
			const size_t sz = static_cast<size_t>(bpp * size.x * size.y);
			pixels			= reinterpret_cast<uint8*>(SFG_MALLOC(sz));
			stream.read_to_raw(pixels, sz);
		}
	}
}