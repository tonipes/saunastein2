// Copyright (c) 2025 Inan Evin

#include "animation.hpp"
#include "animation_raw.hpp"
#include "memory/chunk_allocator.hpp"
#include "world/world.hpp"
#include "io/log.hpp"

namespace SFG
{
	namespace
	{
		template <typename T> uint16 find_segment(const T* keys, uint16 count, float time)
		{
			// assume time is clamped between front/back and count >= 2
			uint16 lo = 0;
			uint16 hi = count - 1;
			while (hi - lo > 1)
			{
				uint16 mid = (lo + hi) >> 1;
				if (time < keys[mid].time)
					hi = mid;
				else
					lo = mid;
			}
			return lo; // segment [lo, lo+1]
		}

	}
	void animation_channel_v3::create_from_loader(const animation_channel_v3_raw& raw, chunk_allocator32& alloc)
	{
		interpolation = raw.interpolation;
		node_index	  = raw.node_index;

		if (interpolation == animation_interpolation::cubic_spline)
		{
			keyframes_count = static_cast<uint16>(raw.keyframes_spline.size());

			if (keyframes_count != 0)
			{
				keyframes							 = alloc.allocate<animation_keyframe_v3_spline>(keyframes_count);
				animation_keyframe_v3_spline* ptr_kf = alloc.get<animation_keyframe_v3_spline>(keyframes);
				for (uint16 i = 0; i < keyframes_count; i++)
					ptr_kf[i] = raw.keyframes_spline[i];
			}
		}
		else
		{
			keyframes_count = static_cast<uint16>(raw.keyframes.size());
			if (keyframes_count != 0)
			{
				keyframes					  = alloc.allocate<animation_keyframe_v3>(keyframes_count);
				animation_keyframe_v3* ptr_kf = alloc.get<animation_keyframe_v3>(keyframes);

				for (uint16 i = 0; i < keyframes_count; i++)
					ptr_kf[i] = raw.keyframes[i];
			}
		}
	}

	void animation_channel_v3::destroy(chunk_allocator32& alloc)
	{
		if (keyframes.size != 0)
			alloc.free(keyframes);
		keyframes		= {};
		keyframes_count = 0;
	}

	vector3 animation_channel_v3::sample(float time, chunk_allocator32& alloc) const
	{
		if (keyframes.size == 0)
			return vector3::zero;

		if (interpolation == animation_interpolation::cubic_spline)
		{
			animation_keyframe_v3_spline*		ptr	  = alloc.get<animation_keyframe_v3_spline>(keyframes);
			const animation_keyframe_v3_spline& front = ptr[0];
			const animation_keyframe_v3_spline& back  = ptr[keyframes_count - 1];
			if (time <= front.time)
				return front.value;
			if (time >= back.time)
				return back.value;

			uint16 i = 0;
			while (i < keyframes_count - 1 && time > ptr[i + 1].time)
				++i;

			const auto& kf0 = ptr[i];
			const auto& kf1 = ptr[i + 1];

			float t0 = kf0.time;
			float t1 = kf1.time;

			// normalized time
			float localT = (time - t0) / (t1 - t0);

			// cubic Hermite spline interpolation.
			float t	 = localT;
			float t2 = t * t;
			float t3 = t2 * t;

			float h00 = 2.0f * t3 - 3.0f * t2 + 1.0f;
			float h10 = t3 - 2.0f * t2 + t;
			float h01 = -2.0f * t3 + 3.0f * t2;
			float h11 = t3 - t2;

			return h00 * kf0.value + h10 * kf0.out_tangent * (t1 - t0) + h01 * kf1.value + h11 * kf1.in_tangent * (t1 - t0);
		}

		animation_keyframe_v3*		 ptr   = alloc.get<animation_keyframe_v3>(keyframes);
		const animation_keyframe_v3& front = ptr[0];
		const animation_keyframe_v3& back  = ptr[keyframes_count - 1];

		if (time <= front.time)
			return front.value;
		if (time >= back.time)
			return back.value;

		uint16 i = 0;
		while (i < keyframes_count - 1 && time > ptr[i + 1].time)
			++i;

		const auto& kf0 = ptr[i];
		const auto& kf1 = ptr[i + 1];

		float t0 = kf0.time;
		float t1 = kf1.time;

		float localT = (time - t0) / (t1 - t0);

		switch (interpolation)
		{
		case animation_interpolation::linear:
			return kf0.value + (kf1.value - kf0.value) * localT;

		case animation_interpolation::step:
			return kf0.value;
		default:
			return vector3::zero;
		}
	}

	void animation_channel_q::create_from_loader(const animation_channel_q_raw& raw, chunk_allocator32& alloc)
	{
		interpolation = raw.interpolation;
		node_index	  = raw.node_index;

		if (interpolation == animation_interpolation::cubic_spline)
		{
			keyframes_count = static_cast<uint16>(raw.keyframes_spline.size());

			if (keyframes_count != 0)
			{
				keyframes							= alloc.allocate<animation_keyframe_q_spline>(keyframes_count);
				animation_keyframe_q_spline* ptr_kf = alloc.get<animation_keyframe_q_spline>(keyframes);
				for (uint16 i = 0; i < keyframes_count; i++)
					ptr_kf[i] = raw.keyframes_spline[i];
			}
		}
		else
		{
			keyframes_count = static_cast<uint16>(raw.keyframes.size());

			if (keyframes_count != 0)
			{
				keyframes					 = alloc.allocate<animation_keyframe_q>(keyframes_count);
				animation_keyframe_q* ptr_kf = alloc.get<animation_keyframe_q>(keyframes);
				for (uint16 i = 0; i < keyframes_count; i++)
					ptr_kf[i] = raw.keyframes[i];
			}
		}
	}

	void animation_channel_q::destroy(chunk_allocator32& alloc)
	{
		if (keyframes.size != 0)
			alloc.free(keyframes);
		keyframes		= {};
		keyframes_count = 0;
	}

	quat animation_channel_q::sample(float time, chunk_allocator32& alloc) const
	{
		if (keyframes.size == 0)
			return quat::identity;

		if (interpolation == animation_interpolation::cubic_spline)
		{
			animation_keyframe_q_spline*	   ptr	 = alloc.get<animation_keyframe_q_spline>(keyframes);
			const animation_keyframe_q_spline& front = ptr[0];
			const animation_keyframe_q_spline& back	 = ptr[keyframes_count - 1];

			if (time <= front.time)
				return front.value;
			if (time >= back.time)
				return back.value;

			uint16 i = 0;
			while (i < keyframes_count - 1 && time > ptr[i + 1].time)
				++i;

			const auto& kf0 = ptr[i];
			const auto& kf1 = ptr[i + 1];

			float t0 = kf0.time;
			float t1 = kf1.time;

			const quat& q0			= kf0.value;
			const quat& q1			= kf1.value;
			const quat& tangentIn0	= kf0.in_tangent;
			const quat& tangentOut0 = kf0.out_tangent;

			float localT = (time - t0) / (t1 - t0);
			float t		 = localT;
			float t2	 = t * t;
			float t3	 = t2 * t;

			float h00 = 2.0f * t3 - 3.0f * t2 + 1.0f;
			float h10 = t3 - 2.0f * t2 + t;
			float h01 = -2.0f * t3 + 3.0f * t2;
			float h11 = t3 - t2;

			return h00 * q0 + h10 * tangentOut0 * (t1 - t0) + h01 * q1 + h11 * tangentIn0 * (t1 - t0);
		}

		if (keyframes_count == 0)
			return quat::identity;

		animation_keyframe_q*		ptr	  = alloc.get<animation_keyframe_q>(keyframes);
		const animation_keyframe_q& front = ptr[0];
		const animation_keyframe_q& back  = ptr[keyframes_count - 1];

		if (time <= front.time)
			return front.value;
		if (time >= back.time)
			return back.value;

		uint16 i = 0;
		while (i < keyframes_count - 1 && time > ptr[i + 1].time)
			++i;

		const auto& kf0 = ptr[i];
		const auto& kf1 = ptr[i + 1];

		float t0 = kf0.time;
		float t1 = kf1.time;

		const quat& q0 = kf0.value;
		const quat& q1 = kf1.value;

		float localT = (time - t0) / (t1 - t0);

		switch (interpolation)
		{
		case animation_interpolation::linear:
			return quat::slerp(q0, q1, localT);

		case animation_interpolation::step:
			return q0;
		default:
			return quat::identity;
		}
	}

	void animation::create_from_loader(const animation_raw& raw, world& w, resource_handle handle)
	{
		resource_manager&  rm	 = w.get_resource_manager();
		chunk_allocator32& alloc = rm.get_aux();

#ifndef SFG_STRIP_DEBUG_NAMES
		if (!raw.name.empty())
			_name = alloc.allocate_text(raw.name);
#endif

		_duration = raw.duration;

		const uint32 position_count = static_cast<uint32>(raw.position_channels.size());

		if (position_count != 0)
		{
			_position_channels		  = alloc.allocate<animation_channel_v3>(position_count);
			animation_channel_v3* ptr = alloc.get<animation_channel_v3>(_position_channels);

			for (uint32 i = 0; i < position_count; i++)
			{
				const animation_channel_v3_raw& ch = raw.position_channels[i];
				animation_channel_v3&			rt = ptr[i];
				rt.create_from_loader(ch, alloc);
			}
		}

		const uint32 rotation_count = static_cast<uint32>(raw.rotation_channels.size());

		if (rotation_count != 0)
		{
			_rotation_channels		 = alloc.allocate<animation_channel_q>(rotation_count);
			animation_channel_q* ptr = alloc.get<animation_channel_q>(_rotation_channels);

			for (uint32 i = 0; i < rotation_count; i++)
			{
				const animation_channel_q_raw& ch = raw.rotation_channels[i];
				animation_channel_q&		   rt = ptr[i];
				rt.create_from_loader(ch, alloc);
			}
		}
		const uint32 scale_count = static_cast<uint32>(raw.scale_channels.size());
		if (scale_count != 0)
		{

			_scale_channels			  = alloc.allocate<animation_channel_v3>(scale_count);
			animation_channel_v3* ptr = alloc.get<animation_channel_v3>(_scale_channels);

			for (uint32 i = 0; i < scale_count; i++)
			{
				const animation_channel_v3_raw& ch = raw.scale_channels[i];
				animation_channel_v3&			rt = ptr[i];
				rt.create_from_loader(ch, alloc);
			}
		}

		_position_count = static_cast<uint16>(position_count);
		_rotation_count = static_cast<uint16>(rotation_count);
		_scale_count	= static_cast<uint16>(scale_count);
	}

	void animation::destroy(world& w, resource_handle handle)
	{
		resource_manager&  rm	 = w.get_resource_manager();
		chunk_allocator32& alloc = rm.get_aux();

#ifndef SFG_STRIP_DEBUG_NAMES
		if (_name.size != 0)
			alloc.free(_name);
		_name = {};
#endif

		if (_position_channels.size != 0)
		{
			animation_channel_v3* ptr = alloc.get<animation_channel_v3>(_position_channels);
			for (uint16 i = 0; i < _position_count; i++)
			{
				animation_channel_v3& ch = ptr[i];
				ch.destroy(alloc);
			}
		}

		if (_rotation_channels.size != 0)
		{
			animation_channel_q* ptr = alloc.get<animation_channel_q>(_rotation_channels);
			for (uint16 i = 0; i < _rotation_count; i++)
			{
				animation_channel_q& ch = ptr[i];
				ch.destroy(alloc);
			}
		}

		if (_scale_channels.size != 0)
		{
			animation_channel_v3* ptr = alloc.get<animation_channel_v3>(_scale_channels);
			for (uint16 i = 0; i < _scale_count; i++)
			{
				animation_channel_v3& ch = ptr[i];
				ch.destroy(alloc);
			}
		}

		_position_channels = {};
		_rotation_channels = {};
		_scale_channels	   = {};
		_position_count = _rotation_count = _scale_count = 0;
	}

}