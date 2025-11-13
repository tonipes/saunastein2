// Copyright (c) 2025 Inan Evin

#ifdef JPH_DEBUG_RENDERER

#include "physics_debug_renderer.hpp"
#include "physics/physics_convert.hpp"

namespace SFG
{
	physics_debug_renderer::physics_debug_renderer()
	{
		_triangle_vertices = new double_buffered_swap<MAX_TRI_VERTICES_SIZE>();
		_triangle_indices  = new double_buffered_swap<MAX_TRI_INDICES_SIZE>();
		_line_vertices	   = new double_buffered_swap<MAX_LINE_VERTICES_SIZE>();
		_line_indices	   = new double_buffered_swap<MAX_LINE_INDICES_SIZE>();

		JPH::DebugRenderer::Initialize();

	}

	physics_debug_renderer::~physics_debug_renderer()
	{
		delete _triangle_vertices;
		delete _triangle_indices;
		delete _line_vertices;
		delete _line_indices;
	}

	void physics_debug_renderer::DrawLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo, JPH::ColorArg inColor)
	{

		const vector3	from	  = from_jph_vec3(inFrom);
		const vector3	to		  = from_jph_vec3(inTo);
		constexpr float thickness = 1.0f;

		const vertex_3d_line vertices[4] = {{
												.pos	   = from,
												.next_pos  = to,
												.color	   = vector4(inColor.r, inColor.g, inColor.b, inColor.a),
												.direction = thickness,
											},
											{
												.pos	   = from,
												.next_pos  = to,
												.color	   = vector4(inColor.r, inColor.g, inColor.b, inColor.a),
												.direction = -thickness,
											},
											{
												.pos	   = to,
												.next_pos  = from,
												.color	   = vector4(inColor.r, inColor.g, inColor.b, inColor.a),
												.direction = thickness,
											},
											{
												.pos	   = to,
												.next_pos  = from,
												.color	   = vector4(inColor.r, inColor.g, inColor.b, inColor.a),
												.direction = -thickness,
											}};

		const uint32 idx_begin	= _vertex_count_line * 6;
		const uint32 indices[6] = {idx_begin, idx_begin + 1, idx_begin + 2, idx_begin + 2, idx_begin + 3, idx_begin};

		_line_vertices->write(vertices, static_cast<size_t>(_vertex_count_line) * sizeof(vertex_3d_line), sizeof(vertex_3d_line) * 4);
		_line_indices->write(indices, idx_begin * sizeof(uint32), sizeof(uint32) * 6);
		_vertex_count_line += 4;
	}

	void physics_debug_renderer::DrawTriangle(JPH::RVec3Arg inV1, JPH::RVec3Arg inV2, JPH::RVec3Arg inV3, JPH::ColorArg inColor, JPH::DebugRenderer::ECastShadow inCastShadow)
	{
		_vertex_count_tri++;

		const vertex_simple vertices[3] = {{
											   .pos	  = from_jph_vec3(inV1),
											   .color = vector4(inColor.r, inColor.g, inColor.b, inColor.a),
										   },
										   {
											   .pos	  = from_jph_vec3(inV2),
											   .color = vector4(inColor.r, inColor.g, inColor.b, inColor.a),
										   },
										   {
											   .pos	  = from_jph_vec3(inV3),
											   .color = vector4(inColor.r, inColor.g, inColor.b, inColor.a),
										   }};

		const uint32 idx_begin	= _vertex_count_tri * 3;
		const uint32 indices[3] = {idx_begin, idx_begin + 1, idx_begin + 2};
		_triangle_vertices->write(vertices, static_cast<size_t>(_vertex_count_tri) * sizeof(vertex_simple), sizeof(vertex_simple) * 3);
		_triangle_indices->write(indices, idx_begin * sizeof(uint32), sizeof(uint32) * 3);
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
