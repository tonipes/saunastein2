// Copyright (c) 2025 Inan Evin

#pragma once

namespace SFG
{

	// -----------------------------------------------------------------------------
	// entities
	// -----------------------------------------------------------------------------

#define MAX_ENTITIES 100000

	// -----------------------------------------------------------------------------
	// resources
	// -----------------------------------------------------------------------------

#define MAX_WORLD_RESOURCES_AUX_MEMORY 1024 * 1024 * 4
#define MAX_WORLD_MODEL_INSTANCES	   1024
#define MAX_WORLD_TEXTURES			   32
#define MAX_WORLD_MODELS			   32
#define MAX_WORLD_MESHES			   256
#define MAX_WORLD_ANIMS				   32
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

#define MAX_WORLD_COMPONENTS_AUX_MEMORY 1024 * 512
#define MAX_WORLD_COMP_CAMERAS			32
#define MAX_WORLD_COMP_DIR_LIGHTS		8
#define MAX_WORLD_COMP_POINT_LIGHTS		128
#define MAX_WORLD_COMP_SPOT_LIGHTS		128
#define MAX_WORLD_COMP_MESH_INSTANCES	1024 * 10
#define MAX_WORLD_COMP_MODEL_INSTANCES	1024
#define MAX_WORLD_COMP_AMBIENT			1
#define MAX_WORLD_COMP_PHYSICS			1000
#define MAX_WORLD_COMP_AUDIO			256
#define MAX_WORLD_COMP_CANVAS			32
#define MAX_WORLD_COMP_ANIMS			2048
#define MAX_WORLD_SKELETON_JOINTS		32

	// -----------------------------------------------------------------------------
	// animations
	// -----------------------------------------------------------------------------
#define MAX_WORLD_BLEND_STATE_ANIMS		   5
#define MAX_WORLD_ANIM_GRAPH_STATES		   4096
#define MAX_WORLD_ANIM_GRAPH_STATE_SAMPLES 4096
#define MAX_WORLD_ANIM_GRAPH_TRANSITION	   4096
#define MAX_WORLD_ANIM_GRAPH_PARAMETER	   4096
#define MAX_WORLD_ANIM_GRAPH_MASK		   32

	// -----------------------------------------------------------------------------
	// gfx
	// -----------------------------------------------------------------------------

#define MAX_RENDERABLES_ALL			   1024 * 20
#define MAX_DRAW_CALLS_CANVAS_2D	   1024
#define MAX_DRAW_CALLS_FORWARD		   1024 * 2
#define MAX_DRAW_CALLS_ALL_OBJECTS	   2048 * 10
#define MAX_DRAW_CALLS_OPAQUE		   1024 * 10
#define PASS_ALLOC_SIZE_CANVAS_2D	   MAX_DRAW_CALLS_CANVAS_2D * 64
#define PASS_ALLOC_SIZE_FORWARD		   MAX_DRAW_CALLS_FORWARD * 64
#define PASS_ALLOC_SIZE_ALL_OBJECTS	   MAX_DRAW_CALLS_ALL_OBJECTS * 64
#define PASS_ALLOC_SIZE_OPAQUE		   MAX_DRAW_CALLS_OPAQUE * 64
#define MAX_GPU_BONES				   1024 * 100
#define MAX_GPU_ENTITIES			   1024 * 100
#define MAX_GPU_SHADOW_DATA			   64
#define MAX_SHADOW_CASCADES			   8
#define LIGHT_CULLING_ENERGY_THRESHOLD 0.01f

}
