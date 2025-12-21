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

#ifdef JPH_DEBUG_RENDERER

#include "physics_debug_renderer.hpp"
#include "physics/physics_convert.hpp"

namespace SFG
{
	physics_debug_renderer::physics_debug_renderer()
	{
		_triangle_vertices.init(MAX_TRI_VERTICES_SIZE, alignof(vertex_simple));
		_triangle_indices.init(MAX_TRI_INDICES_SIZE, alignof(uint32));
		_line_vertices.init(MAX_LINE_VERTICES_SIZE, alignof(vertex_3d_line));
		_line_indices.init(MAX_LINE_INDICES_SIZE, alignof(uint32));

		JPH::DebugRenderer::Initialize();
	}

	physics_debug_renderer::~physics_debug_renderer()
	{
		_triangle_vertices.uninit();
		_triangle_indices.uninit();
		_line_vertices.uninit();
		_line_indices.uninit();
	}

	void physics_debug_renderer::DrawLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo, JPH::ColorArg inColor)
	{

		const vector3	from	  = from_jph_vec3(inFrom);
		const vector3	to		  = from_jph_vec3(inTo);
		constexpr float thickness = 0.25f;

		const vertex_3d_line vertices[4] = {{
												.pos	   = from,
												.next_pos  = to,
												.color	   = vector4(inColor.r, inColor.g, inColor.b, inColor.a) / 255.0f,
												.direction = thickness,
											},
											{
												.pos	   = from,
												.next_pos  = to,
												.color	   = vector4(inColor.r, inColor.g, inColor.b, inColor.a) / 255.0f,
												.direction = -thickness,
											},
											{
												.pos	   = to,
												.next_pos  = from,
												.color	   = vector4(inColor.r, inColor.g, inColor.b, inColor.a) / 255.0f,
												.direction = thickness,
											},
											{
												.pos	   = to,
												.next_pos  = from,
												.color	   = vector4(inColor.r, inColor.g, inColor.b, inColor.a) / 255.0f,
												.direction = -thickness,
											}};

		const uint32 idx_begin	= _vertex_count_line;
		const uint32 indices[6] = {idx_begin, idx_begin + 1, idx_begin + 2, idx_begin + 2, idx_begin + 3, idx_begin};

		_line_vertices.write(vertices, static_cast<size_t>(_vertex_count_line) * sizeof(vertex_3d_line), sizeof(vertex_3d_line) * 4);
		_line_indices.write(indices, (_vertex_count_line / 4) * 6 * sizeof(uint32), sizeof(uint32) * 6);
		_vertex_count_line += 4;
	}

	void physics_debug_renderer::DrawTriangle(JPH::RVec3Arg inV1, JPH::RVec3Arg inV2, JPH::RVec3Arg inV3, JPH::ColorArg inColor, JPH::DebugRenderer::ECastShadow inCastShadow)
	{
		const vertex_simple vertices[3] = {{
											   .pos	  = from_jph_vec3(inV1),
											   .color = vector4(inColor.r, inColor.g, inColor.b, inColor.a) / 255.0f,
										   },
										   {
											   .pos	  = from_jph_vec3(inV2),
											   .color = vector4(inColor.r, inColor.g, inColor.b, inColor.a) / 255.0f,
										   },
										   {
											   .pos	  = from_jph_vec3(inV3),
											   .color = vector4(inColor.r, inColor.g, inColor.b, inColor.a) / 255.0f,
										   }};

		const uint32 idx_begin	= _vertex_count_tri;
		const uint32 indices[3] = {idx_begin, idx_begin + 1, idx_begin + 2};
		_triangle_vertices.write(vertices, static_cast<size_t>(_vertex_count_tri) * sizeof(vertex_simple), sizeof(vertex_simple) * 3);
		_triangle_indices.write(indices, idx_begin * sizeof(uint32), sizeof(uint32) * 3);
		_vertex_count_tri += 3;
	}

	void physics_debug_renderer::DrawText3D(JPH::RVec3Arg inPosition, const std::string_view& inString, JPH::ColorArg inColor, float inHeight)
	{
	}

	JPH::DebugRenderer::Batch physics_debug_renderer::CreateTriangleBatch(const JPH::DebugRenderer::Triangle* inTriangles, int inTriangleCount)
	{
		BatchImpl* batch = new BatchImpl;
		if (inTriangles == nullptr || inTriangleCount == 0)
			return batch;

		batch->mTriangles.assign(inTriangles, inTriangles + inTriangleCount);
		return batch;
	}

	JPH::DebugRenderer::Batch physics_debug_renderer::CreateTriangleBatch(const JPH::DebugRenderer::Vertex* inVertices, int inVertexCount, const uint32* inIndices, int inIndexCount)
	{
		BatchImpl* batch = new BatchImpl;
		if (inVertices == nullptr || inVertexCount == 0 || inIndices == nullptr || inIndexCount == 0)
			return batch;

		// Convert indexed triangle list to triangle list
		batch->mTriangles.resize(inIndexCount / 3);
		for (size_t t = 0; t < batch->mTriangles.size(); ++t)
		{
			JPH::DebugRenderer::Triangle& triangle = batch->mTriangles[t];
			triangle.mV[0]						   = inVertices[inIndices[t * 3 + 0]];
			triangle.mV[1]						   = inVertices[inIndices[t * 3 + 1]];
			triangle.mV[2]						   = inVertices[inIndices[t * 3 + 2]];
		}

		return batch;
	}

	void physics_debug_renderer::DrawGeometry(JPH::RMat44Arg						 inModelMatrix,
											  const JPH::AABox&						 inWorldSpaceBounds,
											  float									 inLODScaleSq,
											  JPH::ColorArg							 inModelColor,
											  const JPH::DebugRenderer::GeometryRef& inGeometry,
											  JPH::DebugRenderer::ECullMode			 inCullMode,
											  JPH::DebugRenderer::ECastShadow		 inCastShadow,
											  JPH::DebugRenderer::EDrawMode			 inDrawMode)
	{
		// Figure out which LOD to use
		const JPH::DebugRenderer::LOD* lod = inGeometry->mLODs.data();

		// Draw the batch
		const BatchImpl* batch = static_cast<const BatchImpl*>(lod->mTriangleBatch.GetPtr());
		for (const JPH::DebugRenderer::Triangle& triangle : batch->mTriangles)
		{
			JPH::RVec3 v0	 = inModelMatrix * JPH::Vec3(triangle.mV[0].mPosition);
			JPH::RVec3 v1	 = inModelMatrix * JPH::Vec3(triangle.mV[1].mPosition);
			JPH::RVec3 v2	 = inModelMatrix * JPH::Vec3(triangle.mV[2].mPosition);
			JPH::Color color = inModelColor * triangle.mV[0].mColor;

			switch (inDrawMode)
			{
			case EDrawMode::Wireframe:
				DrawLine(v0, v1, color);
				DrawLine(v1, v2, color);
				DrawLine(v2, v0, color);
				break;

			case EDrawMode::Solid:
				DrawTriangle(v0, v1, v2, color, inCastShadow);
				break;
			}
		}
	}

}

#endif
