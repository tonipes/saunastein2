// Copyright (c) 2025 Inan Evin
#pragma once

#include "memory/pool_allocator_gen.hpp"
#include "world/world_constants.hpp"
#include "common/type_id.hpp"
#include "data/hash_map.hpp"
#include "data/string.hpp"
#include "data/static_vector.hpp"
#include "memory/chunk_allocator.hpp"
#include <functional>

#ifdef SFG_TOOLMODE
#include "io/simple_file_watcher.hpp"
#include <vendor/nhlohmann/json_fwd.hpp>
using json = nlohmann::json;
#endif

namespace SFG
{
	struct sampler_desc;
	class world;
	class istream;
	class ostream;
	class meta;

	enum class trait_view_result : uint8
	{
		cont,
		stop
	};

	// -----------------------------------------------------------------------------
	// base impl
	// -----------------------------------------------------------------------------

	struct trait_cache_base
	{
		virtual ~trait_cache_base() = default;

		// Lifecycle
		virtual world_handle add(world_handle entity, world& w)								 = 0;
		virtual world_handle add_from_stream(istream& stream, world_handle entity, world& w) = 0;
		virtual void		 save_to_stream(ostream& stream, world_handle handle, world& w)	 = 0;
		virtual void		 remove(world_handle handle, world& w)							 = 0;
		virtual void		 reset(world& w)												 = 0;
		virtual bool		 is_valid(world_handle handle) const							 = 0;

#ifdef SFG_TOOLMODE
		virtual world_handle add_from_json(const json& j, world_handle entity, world& w) = 0;
		virtual void		 save_to_json(json& j, world_handle handle, world& w)		 = 0;
#endif
		// Accessors
		virtual void*		get_ptr(world_handle h)				= 0;
		virtual const void* get_const_ptr(world_handle h) const = 0;

		// Iteration
		virtual void for_each(void* ctx, trait_view_result (*fn)(void* ctx, void* elem)) noexcept						= 0;
		virtual void for_each_handle(void* ctx, trait_view_result (*fn)(void* ctx, const world_handle&)) const noexcept = 0;
	};

	// -----------------------------------------------------------------------------
	// cache impl
	// -----------------------------------------------------------------------------

	template <typename T, int MAX_COUNT> class trait_cache final : public trait_cache_base
	{
	public:
		using cache_type = trait_cache<T, MAX_COUNT>;
		using pool_type	 = pool_allocator_gen<T, world_id, MAX_COUNT>;

		virtual ~trait_cache() = default;

		// -----------------------------------------------------------------------------
		// lifecycle
		// -----------------------------------------------------------------------------

		virtual world_handle add(world_handle entity, world& w) override
		{
			const world_handle handle = _traits.add();
			T&				   trait  = _traits.get(handle);
			trait._header.entity	  = entity;
			trait._header.own_handle  = handle;
			trait.on_add(w);
			return handle;
		}

		virtual world_handle add_from_stream(istream& stream, world_handle entity, world& w) override
		{
			const world_handle handle = _traits.add();
			T&				   trait  = _traits.get(handle);
			trait._header.entity	  = entity;
			trait._header.own_handle  = handle;
			trait.deserialize(stream, w);
			trait.on_add(w);
			return handle;
		}

#ifdef SFG_TOOLMODE
		virtual world_handle add_from_json(const json& j, world_handle entity, world& w) override
		{
			const world_handle handle = _traits.add();
			T&				   trait  = _traits.get(handle);
			trait._header.entity	  = entity;
			trait._header.own_handle  = handle;
			trait.deserialize_json(j, w);
			trait.on_add(w);
			return handle;
		}

		virtual void save_to_json(json& j, world_handle handle, world& w) override
		{
			T& trait = _traits.get(handle);
			trait.serialize_json(j, w);
		}
#endif

		virtual void save_to_stream(ostream& stream, world_handle handle, world& w) override
		{
			T& trait = _traits.get(handle);
			trait.serialize(stream, w);
		}

		virtual void remove(world_handle handle, world& w) override
		{
			T& trait = _traits.get(handle);
			trait.on_remove(w);
			_traits.remove(handle);
		}

		virtual void reset(world& w) override
		{
			for (T& trait : _traits)
			{
				trait.on_remove(w);
			}

			_traits.reset();
		}

		bool is_valid(world_handle handle) const override
		{
			return _traits.is_valid(handle);
		}

		// -----------------------------------------------------------------------------
		// Accessors
		// -----------------------------------------------------------------------------

		void* get_ptr(world_handle h) override
		{
			return &(_traits.get(h));
		}

		const void* get_const_ptr(world_handle h) const override
		{
			return &(_traits.get(h));
		}

		// -----------------------------------------------------------------------------
		// Iteration
		// -----------------------------------------------------------------------------

		void for_each(void* ctx, trait_view_result (*fn)(void*, void*)) noexcept override
		{
			for (auto it = _traits.begin(); it != _traits.end(); ++it)
			{
				T& obj = *it;
				if (fn(ctx, &obj) == trait_view_result::stop)
					break;
			}
		}

		void for_each_handle(void* ctx, trait_view_result (*fn)(void*, const world_handle&)) const noexcept override
		{
			for (auto it = _traits.handles_begin(); it != _traits.handles_end(); ++it)
			{
				const world_handle& handle = *it;
				if (fn(ctx, handle) == trait_view_result::stop)
					break;
			}
		}

		inline pool_type& get_pool()
		{
			return _traits;
		}

	private:
		pool_type _traits;
	};

	// -----------------------------------------------------------------------------
	// trait manager
	// -----------------------------------------------------------------------------

	class trait_manager
	{
	private:
		struct trait_cache_storage
		{
			trait_cache_base* cache_ptr = nullptr;
			string_id		  type		= 0;
		};

	public:
		trait_manager() = delete;
		trait_manager(world& w);
		~trait_manager();

		void init();
		void uninit();

		template <typename T, int MAX_COUNT> void register_cache()
		{
			trait_cache_base* cache = new trait_cache<T, MAX_COUNT>();
			const string_id	  tid	= type_id<T>::value;

			trait_cache_storage storage = {
				.cache_ptr = cache,
				.type	   = tid,
			};

			_storages.push_back(storage);
		}

		// -----------------------------------------------------------------------------
		// Lifecycle
		// -----------------------------------------------------------------------------
		world_handle add_trait(string_id type, world_handle entity);
		world_handle add_trait_from_stream(string_id type, istream& stream, world_handle entity);
		void		 save_trait_to_stream(string_id type, ostream& stream, world_handle trait);
		void		 remove_trait(string_id type, world_handle entity, world_handle handle);
		bool		 is_valid(string_id type, world_handle handle) const;
		void*		 get_trait(string_id type, world_handle handle) const;

#ifdef SFG_TOOLMODE
		world_handle add_trait_from_json(string_id type, const json& json, world_handle entity);
		void		 save_trait_to_json(string_id type, json& j, world_handle trait);
#endif

		template <typename T> inline world_handle add_trait(world_handle entity)
		{
			const string_id type = type_id<T>::value;
			return add_trait(type, entity);
		}

		template <typename T> inline void remove_trait(world_handle entity, world_handle handle)
		{
			const string_id type = type_id<T>::value;
			remove_trait(type, entity, handle);
		}

		template <typename T> inline bool is_valid(world_handle handle) const
		{
			const string_id type = type_id<T>::value;
			return is_valid(type, handle);
		}

		// -----------------------------------------------------------------------------
		// accessors
		// -----------------------------------------------------------------------------

		template <typename T> inline T& get_trait(world_handle handle) const
		{
			const string_id type	 = type_id<T>::value;
			void*			ptr		 = get_trait(type, handle);
			T*				type_ptr = reinterpret_cast<T*>(ptr);
			return *type_ptr;
		}

		inline chunk_allocator32& get_aux()
		{
			return _aux_memory;
		}

		// -----------------------------------------------------------------------------
		// iteration
		// -----------------------------------------------------------------------------

		template <typename T, typename Fn> inline void view(Fn&& fn)
		{
			const string_id			   type = type_id<T>::value;
			const trait_cache_storage& stg	= get_storage(type);

			auto bounce = +[](void* ctx, void* elem) -> trait_view_result {
				auto* f = static_cast<std::remove_reference_t<Fn>*>(ctx);
				return (*f)(*static_cast<T*>(elem));
			};

			using FnNoRef	= std::remove_reference_t<Fn>;
			FnNoRef fn_copy = std::forward<Fn>(fn);
			stg.cache_ptr->for_each(static_cast<void*>(&fn_copy), bounce);
		}

		template <typename T, typename Fn> inline void view_handles(Fn&& fn)
		{
			const string_id			   type = type_id<T>::value;
			const trait_cache_storage& stg	= get_storage(type);

			auto bounce = +[](void* ctx, const world_handle& handle) -> trait_view_result {
				auto* f = static_cast<std::remove_reference_t<Fn>*>(ctx);
				return (*f)(handle);
			};

			using FnNoRef	= std::remove_reference_t<Fn>;
			FnNoRef fn_copy = std::forward<Fn>(fn);
			stg.cache_ptr->for_each_handle(static_cast<void*>(&fn_copy), bounce);
		}

		template <typename CACHE_TYPE, typename T> inline auto& underlying_pool()
		{
			const string_id			   type	 = type_id<T>::value;
			const trait_cache_storage& stg	 = get_storage(type);
			CACHE_TYPE*				   cache = static_cast<CACHE_TYPE*>(stg.cache_ptr);
			return cache->get_pool();
		}

	private:
		const trait_cache_storage& get_storage(string_id type) const;
		trait_cache_storage&	   get_storage(string_id type);

	private:
		chunk_allocator32			_aux_memory = {};
		vector<trait_cache_storage> _storages	= {};
		world&						_world;
	};
}