// Copyright (c) 2025 Inan Evin
#pragma once

#define NOMINMAX
#include <limits>

namespace SFG
{
#define MAX_RESOURCES		   512
#define MAX_TEXTURES		   512
#define MAX_SAMPLERS		   128
#define MAX_SEMAPHORES		   64
#define MAX_SHADERS			   256
#define MAX_PIPELINE_LAYOUTS   256
#define MAX_SWAPCHAINS		   8
#define MAX_BIND_GROUPS		   512
#define MAX_BIND_LAYOUTS	   128
#define MAX_COMMAND_BUFFERS	   256
#define MAX_QUEUES			   8
#define MAX_DESCRIPTOR_HANDLES 1024
#define COMMANDS_MAX_TID	   25

#define BACK_BUFFER_COUNT 3
#define FRAME_LATENCY	  2

	// 0 discrete, 1 integrated
#define GPU_DEVICE 0

	typedef unsigned short gfx_id;
	typedef unsigned short primitive_index;
	typedef unsigned int   gpu_index;

#define NULL_GFX_ID	   std::numeric_limits<gfx_id>::max()
#define NULL_GPU_INDEX std::numeric_limits<gpu_index>::max()

#define GBUFFER_COLOR_TEXTURES 4
}