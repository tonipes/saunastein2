// Copyright (c) 2025 Inan Evin

#pragma once

namespace SFG
{

	// -----------------------------------------------------------------------------
	// entities
	// -----------------------------------------------------------------------------

#define MAX_ENTITIES 256000

	// -----------------------------------------------------------------------------
	// resources
	// -----------------------------------------------------------------------------

#define MAX_WORLD_RESOURCES_AUX_MEMORY 1024 * 1024 * 32
#define MAX_WORLD_MODEL_INSTANCES	   1024
#define MAX_WORLD_TEXTURES			   32
#define MAX_WORLD_MODELS			   32
#define MAX_WORLD_MESHES			   256
#define MAX_WORLD_ANIMS				   1024
#define MAX_WORLD_SKINS				   12
#define MAX_WORLD_MATERIALS			   20
#define MAX_WORLD_SHADERS			   30
#define MAX_WORLD_AUDIO				   50
#define MAX_WORLD_FONTS				   20
#define MAX_WORLD_PHYSICAL_MATERIALS   30
#define MAX_WORLD_SAMPLERS			   16
#define MAX_TEXTURE_MIPS			   10
#define MAX_MATERIAL_TEXTURES		   8

	// -----------------------------------------------------------------------------
	// traits
	// -----------------------------------------------------------------------------

#define MAX_WORLD_COMPONENTS_AUX_MEMORY 1024 * 1024 * 6
#define MAX_WORLD_COMP_CAMERAS			32
#define MAX_WORLD_COMP_DIR_LIGHTS		8
#define MAX_WORLD_COMP_POINT_LIGHTS		128
#define MAX_WORLD_COMP_SPOT_LIGHTS		128
#define MAX_WORLD_COMP_MESH_INSTANCES	1024 * 10
#define MAX_WORLD_COMP_MODEL_INSTANCES	1024 * 10
#define MAX_WORLD_COMP_AMBIENT			1
#define MAX_WORLD_COMP_PHYSICS			1000
#define MAX_WORLD_COMP_AUDIO			256
#define MAX_WORLD_COMP_CANVAS			32
#define MAX_WORLD_COMP_ANIMS			1024 * 10
#define MAX_WORLD_SKELETON_JOINTS		128

	// -----------------------------------------------------------------------------
	// animations
	// -----------------------------------------------------------------------------
#define MAX_WORLD_BLEND_STATE_ANIMS		   5
#define MAX_WORLD_ANIM_GRAPH_STATES		   MAX_WORLD_COMP_ANIMS * 10
#define MAX_WORLD_ANIM_GRAPH_STATE_SAMPLES MAX_WORLD_COMP_ANIMS * 10
#define MAX_WORLD_ANIM_GRAPH_TRANSITION	   MAX_WORLD_COMP_ANIMS * 20
#define MAX_WORLD_ANIM_GRAPH_PARAMETER	   MAX_WORLD_COMP_ANIMS * 10
#define MAX_WORLD_ANIM_GRAPH_MASK		   32

	// -----------------------------------------------------------------------------
	// gfx
	// -----------------------------------------------------------------------------

#define MAX_RENDERABLES_ALL			   1024 * 20
#define MAX_DRAW_CALLS_CANVAS_2D	   1024
#define MAX_DRAW_CALLS_FORWARD		   1024 * 2
#define MAX_DRAW_CALLS_ALL_OBJECTS	   1024 * 24
#define MAX_DRAW_CALLS_OPAQUE		   1024 * 20
#define PASS_ALLOC_SIZE_CANVAS_2D	   MAX_DRAW_CALLS_CANVAS_2D * 64
#define PASS_ALLOC_SIZE_FORWARD		   MAX_DRAW_CALLS_FORWARD * 64
#define PASS_ALLOC_SIZE_ALL_OBJECTS	   MAX_DRAW_CALLS_ALL_OBJECTS * 64
#define PASS_ALLOC_SIZE_OPAQUE		   MAX_DRAW_CALLS_OPAQUE * 64
#define MAX_GPU_BONES				   MAX_ENTITIES
#define MAX_GPU_ENTITIES			   MAX_ENTITIES
#define MAX_GPU_SHADOW_DATA			   64
#define MAX_SHADOW_CASCADES			   8
#define LIGHT_CULLING_ENERGY_THRESHOLD 0.01f
#define RENDER_STREAM_BATCH_SIZE	   1024 * 1024 * 16 // * 5 default
#define RENDER_STREAM_MAX_BATCHES	   8

}
