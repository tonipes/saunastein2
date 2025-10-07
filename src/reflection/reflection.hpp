// Copyright (c) 2025 Inan Evin

#pragma once
#include "data/hash_map.hpp"
#include "data/vector.hpp"
#include "data/string.hpp"
#include "common/string_id.hpp"
#include "io/assert.hpp"
#include "memory/malloc_allocator_map.hpp"
#include "memory/memory.hpp"

#pragma warning(push)
#pragma warning(disable : 4541)

namespace SFG
{

	struct reflection_function_base
	{
		virtual ~reflection_function_base()	  = default;
		virtual size_t signature_hash() const = 0;
	};

	template <typename RetVal, typename... Args> struct reflection_function : public reflection_function_base
	{
		using FuncType = std::function<RetVal(Args...)>;
		FuncType func;

		reflection_function(FuncType f) : func(std::move(f))
		{
		}

		size_t signature_hash() const override
		{
			return typeid(reflection_function<RetVal, Args...>).hash_code();
		}

		RetVal invoke(Args... args)
		{
			return func(std::forward<Args>(args)...);
		}
	};

	class meta
	{
	public:
		typedef phmap::flat_hash_map<string_id, reflection_function_base*, phmap::priv::hash_default_hash<string_id>, phmap::priv::hash_default_eq<string_id>, malloc_allocator_map<string_id>> alloc_map;

		template <typename RetVal, typename... Args, typename F> meta& add_function(string_id id, F&& f)
		{
			using func_t						= reflection_function<RetVal, Args...>;
			std::function<RetVal(Args...)> func = std::forward<F>(f);

			void*	mem			   = SFG_ALIGNED_MALLOC(alignof(func_t), sizeof(func_t));
			func_t* reflectionFunc = new (mem) func_t(std::move(func));
			_functions[id]		   = reflectionFunc;

			//_functions[id]						= std::make_unique<func_t>(std::move(func));
			return *this;
		}

		template <typename RetVal, typename... Args> RetVal invoke_function(string_id id, Args... args) const
		{
			auto it = _functions.find(id);
			if (it == _functions.end())
				throw std::runtime_error("Function not found");

			auto* ptr = it->second;
			if (!ptr)
				throw std::runtime_error("Empty reflection ptr!");

			// Signature check
			if (ptr->signature_hash() != typeid(reflection_function<RetVal, Args...>).hash_code())
				throw std::runtime_error("Signature mismatch");

			auto* func = static_cast<reflection_function<RetVal, Args...>*>(ptr);
			return func->invoke(std::forward<Args>(args)...);
		}

		reflection_function_base* get_function(string_id id)
		{
			return _functions[id];
		}

		bool has_function(string_id id) const
		{
			return _functions.find(id) != _functions.end();
		}

		inline string_id get_type_id() const
		{
			return _type_id;
		}

		inline const string_id get_tag() const
		{
			return _tag;
		}

		inline uint32 get_type_index() const
		{
			return _type_index;
		}

	private:
		friend class reflection;

		inline void destroy()
		{
			for (auto [sid, ptr] : _functions)
			{
				ptr->~reflection_function_base();
				SFG_ALIGNED_FREE(ptr);
			}
		}

	private:
		alloc_map _functions;
		string_id _type_id	  = 0;
		string_id _tag		  = 0;
		uint32	  _type_index = 0;
	};

	class reflection
	{
	public:
		typedef phmap::flat_hash_map<string_id, meta, phmap::priv::hash_default_hash<string_id>, phmap::priv::hash_default_eq<string_id>, malloc_allocator_map<string_id>> alloc_map;

		static reflection& get()
		{
			static reflection ref;
			return ref;
		}

		~reflection()
		{
			for (auto& [sid, meta] : _metas)
			{
				meta.destroy();
			}
		}

		meta& register_meta(string_id id, uint32 index, const string& tag)
		{
			meta& m		  = _metas[id];
			m._type_id	  = id;
			m._tag		  = TO_SID(tag);
			m._type_index = index;
			return m;
		}

		meta& resolve(string_id id)
		{
			meta& m = _metas.at(id);
			return m;
		}

		meta* try_resolve(string_id id)
		{
			auto it = _metas.find(id);
			return it == _metas.end() ? nullptr : &(it->second);
		}

		const alloc_map& get_metas() const
		{
			return _metas;
		}

		meta* find_by_tag(const char* tag);

	private:
		alloc_map _metas;
	};
};

#pragma warning(pop)
