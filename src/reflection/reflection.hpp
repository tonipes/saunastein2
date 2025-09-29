// Copyright (c) 2025 Inan Evin

#pragma once
#include "data/hash_map.hpp"
#include "data/vector.hpp"
#include "data/string.hpp"
#include "common/string_id.hpp"
#include "io/assert.hpp"

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
		template <typename RetVal, typename... Args, typename F> meta& add_function(string_id id, F&& f)
		{
			std::function<RetVal(Args...)> func = std::forward<F>(f);
			_functions[id]						= std::make_unique<reflection_function<RetVal, Args...>>(std::move(func));
			return *this;
		}

		template <typename RetVal, typename... Args> RetVal invoke_function(string_id id, Args... args) const
		{
			auto it = _functions.find(id);
			if (it == _functions.end())
				throw std::runtime_error("Function not found");

			auto* ptr = it->second.get();
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
			return _functions[id].get();
		}

		bool has_function(string_id id) const
		{
			return _functions.find(id) != _functions.end();
		}

		inline string_id get_type_id() const
		{
			return _type_id;
		}

		inline const string& get_tag() const
		{
			return _tag;
		}

	private:
		friend class reflection;

		hash_map<string_id, std::unique_ptr<reflection_function_base>> _functions;
		string_id													   _type_id = 0;
		string														   _tag		= "";
	};

	class reflection
	{
	public:
		static reflection& get()
		{
			static reflection ref;
			return ref;
		}

		meta& register_meta(string_id id, const char* tag)
		{
			meta& m	   = _metas[id];
			m._type_id = id;
			m._tag	   = tag;
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

		const hash_map<string_id, meta>& get_metas() const
		{
			return _metas;
		}

		meta* find_by_tag(const char* tag);

	private:
		hash_map<string_id, meta> _metas;
	};
};

#pragma warning(pop)
