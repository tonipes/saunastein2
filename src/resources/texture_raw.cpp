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

#include "texture_raw.hpp"
#include "data/ostream.hpp"
#include "data/istream.hpp"
#include "memory/memory_tracer.hpp"

#ifdef SFG_TOOLMODE
#include "io/file_system.hpp"
#include "io/log.hpp"
#include "io/assert.hpp"
#include "serialization/serialization.hpp"
#include "math/vector2ui16.hpp"
#include "math/math.hpp"
#include "gfx/common/format.hpp"
#include "gfx/util/image_util.hpp"
#include <fstream>
#include <vendor/nhlohmann/json.hpp>
using json = nlohmann::json;

#endif

namespace SFG
{
	void texture_raw::serialize(ostream& stream) const
	{
		const uint16 count = static_cast<uint16>(buffers.size());
		stream << name;
		stream << source;
		stream << texture_format;
		stream << sid;
		stream << count;

		for (const texture_buffer& b : buffers)
		{
			stream << b.size;
			stream << b.bpp;
			stream.write_raw(b.pixels, static_cast<size_t>(b.size.x * b.size.y * b.bpp));
		}
	}

	void texture_raw::deserialize(istream& stream)
	{
		uint16 count = 0;
		stream >> name;
		stream >> source;
		stream >> texture_format;
		stream >> sid;
		stream >> count;
		buffers.resize(count);

		for (uint16 i = 0; i < count; i++)
		{
			texture_buffer& b = buffers[i];
			stream >> b.size;
			stream >> b.bpp;

			const size_t pixels_size = b.size.x * b.size.y * b.bpp;

			SFG_ASSERT(pixels_size != 0);
			b.pixels = reinterpret_cast<uint8*>(SFG_MALLOC(pixels_size));
			PUSH_ALLOCATION_SZ(pixels_size);
			stream.read_to_raw(b.pixels, pixels_size);
		}

		SFG_INFO("Created texture from buffer: {0}", name);
	}

	void texture_raw::load_from_data(uint8* base, const vector2ui16& size, uint8 txt_format, bool generate_mips)
	{
		const format fmt	   = static_cast<format>(txt_format);
		const uint8	 bpp	   = format_get_bpp(fmt);
		const bool	 is_linear = format_is_linear(fmt);
		const uint8	 channels  = format_get_channels(fmt);
		texture_format		   = txt_format;

		const texture_buffer b = {
			.pixels = reinterpret_cast<uint8*>(base),
			.size	= size,
			.bpp	= bpp,
		};

		const uint8 count = generate_mips ? math::min(image_util::calculate_mip_levels(size.x, size.y), (uint8)MAX_TEXTURE_MIPS) : 1;
		buffers.resize(count);
		buffers[0] = b;

		if (generate_mips)
			image_util::generate_mips(buffers.data(), count, image_util::mip_gen_filter::box, channels, is_linear, false);
	}

#ifdef SFG_TOOLMODE
	bool texture_raw::load_from_file(const char* relative_file, const char* base_path)
	{
		const string target_path = base_path + string(relative_file);
		if (!file_system::exists(target_path.c_str()))
		{
			SFG_ERR("File don't exist! {0}", target_path.c_str());
			return false;
		}

		try
		{
			std::ifstream f(target_path);
			json		  json_data = json::parse(f);
			f.close();

			texture_format = static_cast<uint8>(json_data.value<format>("format", format::r8g8b8a8_srgb));
			source		   = json_data.value<string>("source", "");
			sid			   = TO_SID(relative_file);
			name		   = relative_file;

			const bool	 mips		 = json_data.value<uint8>("gen_mips", 0);
			const format fmt		 = static_cast<format>(static_cast<format>(texture_format));
			const uint8	 channels	 = format_get_channels(fmt);
			const uint8	 bpp		 = format_get_bpp(fmt);
			const bool	 is_linear	 = format_is_linear(fmt);
			const string full_source = base_path + source;
			SFG_ASSERT(file_system::exists(full_source.c_str()));
			SFG_ASSERT(fmt != format::undefined);

			vector2ui16 size = vector2ui16::zero;
			void*		data = image_util::load_from_file_ch(full_source.c_str(), size, channels);

			if (data == nullptr)
			{
				SFG_ERR("Failed loading pixel data for texture: {0}", target_path);
				return false;
			}

			PUSH_ALLOCATION_SZ(size.x * size.y * bpp);

			const texture_buffer b = {
				.pixels = reinterpret_cast<uint8*>(data),
				.size	= size,
				.bpp	= bpp,
			};

			const uint8 count = mips ? math::min(image_util::calculate_mip_levels(size.x, size.y), (uint8)MAX_TEXTURE_MIPS) : 1;

			buffers.resize(count);
			buffers[0] = b;

			if (mips == 1)
				image_util::generate_mips(buffers.data(), count, image_util::mip_gen_filter::box, channels, is_linear, false);
		}
		catch (std::exception e)
		{
			SFG_ERR("Exception: {0}", e.what());
			return false;
		}

		SFG_INFO("Created texture from file: {0}", name);

		return true;
	}

	bool texture_raw::load_from_cache(const char* cache_folder_path, const char* relative_path, const char* extension)
	{
		const string sid_str		 = std::to_string(TO_SID(relative_path));
		const string meta_cache_path = cache_folder_path + sid_str + "_meta" + extension;
		const string data_cache_path = cache_folder_path + sid_str + "_data" + extension;

		if (!file_system::exists(meta_cache_path.c_str()))
			return false;

		if (!file_system::exists(data_cache_path.c_str()))
			return false;

		istream stream = serialization::load_from_file(meta_cache_path.c_str());

		string file_path				  = "";
		string source_path				  = "";
		uint64 saved_file_last_modified	  = 0;
		uint64 saved_source_last_modified = 0;
		stream >> file_path;
		stream >> source_path;
		stream >> saved_file_last_modified;
		stream >> saved_source_last_modified;

		stream.destroy();

		const uint64 file_last_modified = file_system::get_last_modified_ticks(file_path);
		const uint64 src_last_modified	= file_system::get_last_modified_ticks(source_path);

		if (file_last_modified != saved_file_last_modified || src_last_modified != saved_source_last_modified)
			return false;

		stream = serialization::load_from_file(data_cache_path.c_str());
		deserialize(stream);
		stream.destroy();
		return true;
	}

	void texture_raw::save_to_cache(const char* cache_folder_path, const char* resource_directory_path, const char* extension) const
	{
		const string sid_str			= std::to_string(TO_SID(name));
		const string file_path			= resource_directory_path + name;
		const string source_path		= resource_directory_path + source;
		const uint64 file_last_modified = file_system::get_last_modified_ticks(file_path);
		const uint64 src_last_modified	= file_system::get_last_modified_ticks(source_path);

		const string meta_cache_path = cache_folder_path + sid_str + "_meta" + extension;
		const string data_cache_path = cache_folder_path + sid_str + "_data" + extension;

		ostream out_stream;
		out_stream << file_path;
		out_stream << source_path;
		out_stream << file_last_modified;
		out_stream << src_last_modified;
		serialization::save_to_file(meta_cache_path.c_str(), out_stream);

		out_stream.shrink(0);
		serialize(out_stream);
		serialization::save_to_file(data_cache_path.c_str(), out_stream);

		out_stream.destroy();
	}

	void texture_raw::get_dependencies(vector<string>& out_deps) const
	{
		out_deps.push_back(source);
	}

#endif

}
