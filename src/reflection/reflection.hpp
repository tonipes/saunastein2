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

#pragma once
#include "data/hash_map.hpp"
#include "data/vector.hpp"
#include "data/string.hpp"
#include "common/string_id.hpp"
#include "common/type_id.hpp"
#include "io/assert.hpp"
#include "reflection/common_reflection.hpp"
#include "memory/malloc_allocator_map.hpp"
#include "memory/malloc_allocator_stl.hpp"
#include "memory/memory.hpp"

#pragma warning(push)
#pragma warning(disable : 4541)

namespace SFG
{
	using malloc_string = std::basic_string<char, std::char_traits<char>, malloc_allocator_stl<char>>;

	class field_value
	{
	public:
		field_value() {};
		field_value(void* addr) : _ptr(addr) {};
		template <typename T> T get_value()
		{
			return cast<T>();
		}

		template <typename T> void set_value(T t)
		{
			cast<T>() = t;
		}

		template <typename T> T cast()
		{
			return *static_cast<T*>(_ptr);
		}

		template <typename T> T& cast_ref()
		{
			return *static_cast<T*>(_ptr);
		}

		template <typename T> T* cast_ptr()
		{
			return static_cast<T*>(_ptr);
		}

		void* get_ptr() const
		{
			return _ptr;
		}

	private:
		template <typename T, typename U> friend class field;
		void* _ptr = nullptr;
	};

	class field_base
	{
	public:
		typedef vector<malloc_string, malloc_allocator_stl<malloc_string>> enum_vec;

		field_base()		  = default;
		virtual ~field_base() = default;

		virtual field_value value(void* obj) const = 0;
		virtual size_t		get_type_size() const  = 0;

		enum_vec			 _enum_list	  = {};
		malloc_string		 _title		  = "";
		malloc_string		 _tooltip	  = "";
		string_id			 _sid		  = 0;
		string_id			 _sub_type_id = 0;
		float				 _min		  = 0.0f;
		float				 _max		  = 0.0f;
		reflected_field_type _type		  = reflected_field_type::rf_float;
		uint8				 _is_list	  = 0;
		uint8				 _no_ui		  = 0;
		uint8				 _clamped	  = 0;
	};

	template <typename T, class C> class field : public field_base
	{
	public:
		field()			 = default;
		virtual ~field() = default;

		virtual size_t get_type_size() const override
		{
			return sizeof(T);
		}

		inline virtual field_value value(void* obj) const override
		{
			field_value val;
			val._ptr = &((static_cast<C*>(obj))->*(_var));
			return val;
		}

		T _var = T();
	};

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
		typedef vector<field_base*, malloc_allocator_stl<field_base*>>																															field_vec;

		template <auto DATA, typename Class> field_base* add_field(const string& title, reflected_field_type type, const string& tooltip, float min, float max, string_id sub_type_id = 0, uint8 is_list = 0, uint8 no_ui = 0)
		{
			using ft = field<decltype(DATA), Class>;

			void* mem		= SFG_ALIGNED_MALLOC(alignof(ft), sizeof(ft));
			ft*	  f			= new (mem) ft();
			f->_var			= DATA;
			f->_sid			= TO_SID(title);
			f->_type		= type;
			f->_min			= min;
			f->_max			= max;
			f->_sub_type_id = sub_type_id;
			f->_tooltip		= tooltip;
			f->_title		= title;
			f->_clamped		= 1;
			f->_is_list		= is_list;
			f->_no_ui		= no_ui;
			_fields.push_back(f);
			return f;
		}

		template <auto DATA, typename Class> field_base* add_field(const string& title, reflected_field_type type, const string& tooltip, string_id sub_type_id = 0, uint8 is_list = 0, uint8 no_ui = 0)
		{
			using ft = field<decltype(DATA), Class>;

			void* mem		= SFG_ALIGNED_MALLOC(alignof(ft), sizeof(ft));
			ft*	  f			= new (mem) ft();
			f->_var			= DATA;
			f->_sid			= TO_SID(title);
			f->_type		= type;
			f->_min			= 0.0f;
			f->_max			= 0.0f;
			f->_sub_type_id = sub_type_id;
			f->_tooltip		= tooltip;
			f->_title		= title;
			f->_is_list		= is_list;
			f->_no_ui		= no_ui;
			_fields.push_back(f);
			return f;
		}

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

		inline const malloc_string& get_tag_str() const
		{
			return _tag_str;
		}

		inline uint32 get_type_index() const
		{
			return _type_index;
		}

		inline const field_vec& get_fields() const
		{
			return _fields;
		}

		inline void set_title(const char* t)
		{
			_title = t;
		}

		inline const malloc_string& get_title() const
		{
			return _title;
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

			for (auto f : _fields)
			{
				SFG_ALIGNED_FREE(f);
			}
		}

	private:
		alloc_map	  _functions;
		field_vec	  _fields;
		malloc_string _title	  = "";
		malloc_string _tag_str	  = "";
		string_id	  _type_id	  = 0;
		string_id	  _tag		  = 0;
		uint32		  _type_index = 0;
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
			m._tag_str	  = tag;
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

		const meta* find_by_tag(const char* tag) const;

	private:
		alloc_map _metas;
	};

#define SFG_PP_CONCAT_INNER(a, b) a##b
#define SFG_PP_CONCAT(a, b)		  SFG_PP_CONCAT_INNER(a, b)

	/*
#define REFLECT_FIELD(CLASSNAME, FIELDNAME, TITLE, FIELD_TYPE, TOOLTIP, MIN, MAX)                                                                                                                                                                                  \
	struct reflected_field_##CLASSNAME##FIELDNAME                                                                                                                                                                                                                  \
	{                                                                                                                                                                                                                                                              \
		reflected_field_##CLASSNAME##FIELDNAME()                                                                                                                                                                                                                   \
		{                                                                                                                                                                                                                                                          \
			reflection::get().resolve(type_id<CLASSNAME>::value).add_field<&CLASSNAME::FIELDNAME, CLASSNAME>(TITLE##_hs, FIELD_TYPE, TITLE, TOOLTIP, MIN, MAX);                                                                                                    \
		}                                                                                                                                                                                                                                                          \
	};                                                                                                                                                                                                                                                             \
	inline static reflected_field_##CLASSNAME##FIELDNAME SFG_PP_CONCAT(_ref_inst, __COUNTER__) = reflected_field_##CLASSNAME##FIELDNAME()
	*/

};

#pragma warning(pop)
