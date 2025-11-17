// Copyright (c) 2025 Inan Evin

#pragma once

#ifdef JPH_DEBUG_RENDERER

#include "data/vector.hpp"
#include "resources/vertex.hpp"
#include "resources/common_resources.hpp"
#include "gfx/buffer.hpp"
#include "memory/double_buffered_swap.hpp"

#include <Jolt/Jolt.h>
#include <Jolt/Renderer/DebugRenderer.h>

namespace SFG
{
	class physics_debug_renderer : public JPH::DebugRenderer
	{
	public:
		static constexpr size_t MAX_TRI_VERTICES_SIZE  = sizeof(vertex_simple) * 48000;
		static constexpr size_t MAX_LINE_VERTICES_SIZE = sizeof(vertex_3d_line) * 4000;
		static constexpr size_t MAX_TRI_INDICES_SIZE   = sizeof(uint32) * 48000;
		static constexpr size_t MAX_LINE_INDICES_SIZE  = sizeof(uint32) * 12000;

		physics_debug_renderer();
		virtual ~physics_debug_renderer();

		// -----------------------------------------------------------------------------
		// JPH override
		// -----------------------------------------------------------------------------

		virtual void					  DrawLine(JPH::RVec3Arg inFrom, JPH::RVec3Arg inTo, JPH::ColorArg inColor) override;
		virtual void					  DrawTriangle(JPH::RVec3Arg inV1, JPH::RVec3Arg inV2, JPH::RVec3Arg inV3, JPH::ColorArg inColor, JPH::DebugRenderer::ECastShadow inCastShadow) override;
		virtual void					  DrawText3D(JPH::RVec3Arg inPosition, const std::string_view& inString, JPH::ColorArg inColor, float inHeight) override;
		virtual JPH::DebugRenderer::Batch CreateTriangleBatch(const JPH::DebugRenderer::Triangle* inTriangles, int inTriangleCount) override;
		virtual JPH::DebugRenderer::Batch CreateTriangleBatch(const JPH::DebugRenderer::Vertex* inVertices, int inVertexCount, const uint32* inIndices, int inIndexCount) override;

		virtual void DrawGeometry(JPH::RMat44Arg						 inModelMatrix,
								  const JPH::AABox&						 inWorldSpaceBounds,
								  float									 inLODScaleSq,
								  JPH::ColorArg							 inModelColor,
								  const JPH::DebugRenderer::GeometryRef& inGeometry,
								  JPH::DebugRenderer::ECullMode			 inCullMode	  = JPH::DebugRenderer::ECullMode::CullBackFace,
								  JPH::DebugRenderer::ECastShadow		 inCastShadow = JPH::DebugRenderer::ECastShadow::On,
								  JPH::DebugRenderer::EDrawMode			 inDrawMode	  = JPH::DebugRenderer::EDrawMode::Solid) override;

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		inline void reset()
		{
			_vertex_count_line = 0;
			_vertex_count_tri  = 0;
			_triangle_vertices.swap();
			_triangle_indices.swap();
			_line_vertices.swap();
			_line_indices.swap();
		}

		inline void end()
		{
			_read_vertex_count_line.store(_vertex_count_line, std::memory_order_release);
			_read_vertex_count_tri.store(_vertex_count_tri, std::memory_order_release);
		}

		inline const double_buffered_swap& get_triangle_vertices() const
		{
			return _triangle_vertices;
		}

		inline const double_buffered_swap& get_triangle_indices() const
		{
			return _triangle_indices;
		}

		inline const double_buffered_swap& get_line_vertices() const
		{
			return _line_vertices;
		}

		inline const double_buffered_swap& get_line_indices() const
		{
			return _line_indices;
		}

		inline uint32 get_vertex_count_triangle() const
		{
			return _read_vertex_count_tri.load(std::memory_order_acquire);
		}

		inline uint32 get_vertex_count_line() const
		{
			return _read_vertex_count_line.load(std::memory_order_acquire);
		}

		inline uint32 get_index_count_triangle() const
		{
			return _read_vertex_count_tri.load(std::memory_order_acquire);
		}

		inline uint32 get_index_count_line() const
		{
			return (_read_vertex_count_line.load(std::memory_order_acquire) / 4) * 6;
		}

	private:
		/// Implementation specific batch object
		class BatchImpl : public JPH::RefTargetVirtual
		{
		public:
			JPH_OVERRIDE_NEW_DELETE

			virtual void AddRef() override
			{
				++mRefCount;
			}
			virtual void Release() override
			{
				if (--mRefCount == 0)
					delete this;
			}

			JPH::Array<JPH::DebugRenderer::Triangle> mTriangles;

		private:
			JPH::atomic<uint32> mRefCount = 0;
		};

	private:
		double_buffered_swap _triangle_vertices = {};
		double_buffered_swap _triangle_indices	= {};
		double_buffered_swap _line_vertices		= {};
		double_buffered_swap _line_indices		= {};

		uint32 _vertex_count_line = 0;
		uint32 _vertex_count_tri  = 0;

		atomic<uint32> _read_vertex_count_line = 0;
		atomic<uint32> _read_vertex_count_tri  = 0;
	};
}

#endif