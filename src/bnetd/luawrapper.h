/*
*	Copyright (c) 2009, Anton Burdinuk <clark15b@gmail.com>
*	All rights reserved.
*
*	Redistribution and use in source and binary forms, with or without
*	modification, are permitted provided that the following conditions are met:
*	* Redistributions of source code must retain the above copyright
*	notice, this list of conditions and the following disclaimer.
*	* Redistributions in binary form must reproduce the above copyright
*	notice, this list of conditions and the following disclaimer in the
*	documentation and/or other materials provided with the distribution.
*	* Neither the name of the organization nor the
*	names of its contributors may be used to endorse or promote products
*	derived from this software without specific prior written permission.
*
*	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
*	ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
*	WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
*	DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
*	DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
*	(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
*	LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
*	ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
*	(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
*	SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifdef WITH_LUA
#ifndef INCLUDED_LUAWRAPPER_H
#define INCLUDED_LUAWRAPPER_H

extern "C"
{
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

#include <cstring>
#include <map>
#include <stdexcept>
#include <string>
#include <typeinfo>
#include <vector>

namespace lua
{
	// lua exception

	class exception : public std::exception
	{
	public:
		explicit exception(const std::string s) : m_what(s)
		{
		}

		virtual ~exception() throw()
		{
		}

		virtual const char* what() const throw()
		{
			return m_what.c_str();
		}
	protected:
		std::string m_what;
	};

	// check Lua VM for errors and throw C++ lua::exception
	void throw_lua_exception(lua_State* st, const std::string& addinfo = std::string());

	// Lua VM instance

	class vm
	{
	public:
		vm() : st(nullptr), mutex(0)
		{
		}

		explicit vm(lua_State* _st) : st(_st), mutex(0)
		{
		}

		~vm()
		{
			this->done();
		}

		// create new Lua VM instance
		void initialize();

		// load and execute Lua script from file
		void load_file(const char* file);

		// load and execute Lua statement from string
		void eval(const std::string& stmt, int offset = 0);

		// register external C function
		void reg(const char* name, lua_CFunction func)
		{
			lua_register(st, name, func);
		}
		void reg(const char* package, const luaL_Reg* members)
		{
			luaL_register(st, package, members);
		}

		// destroy Lua VM instance
		void done()
		{
			if (st)
			{
				lua_close(st);
			}
		}

		lua_State* get_st() const
		{
			return st;
		}

		friend class stack;
		friend class bind;
	protected:
		lua_State* st;

		// mutex for lua::bind
		int mutex;

	};


	// Lua stack wrapper (use in custom CFunctions only)

	class stack
	{
	public:
		// initialize stack
		stack() : st(nullptr), st_top(0)
		{
		}
		explicit stack(lua_State* _st) : st(_st), st_top(_st ? lua_gettop(_st) : 0)
		{
		}
		explicit stack(const vm& _vm) : st(_vm.st), st_top(_vm.st ? lua_gettop(_vm.st) : 0)
		{
		}

		// return current stack size
		int size() const
		{
			return lua_gettop(st);
		}

		// find field in table
		void find(const char* name, int index = LUA_GLOBALSINDEX)
		{
			lua_getfield(st, index, name);
		}

		// push to stack scalar
		void push(unsigned int v)
		{
			lua_pushinteger(st, v);
		}
		void push(int v)
		{
			lua_pushinteger(st, v);
		}
		void push_boolean(int v)
		{
			lua_pushboolean(st, v);
		}
		void push(double v)
		{
			lua_pushnumber(st, v);
		}
		void push(const std::string& v)
		{
			lua_pushlstring(st, v.c_str(), v.length());
		}
		void push(const char* v)
		{
			lua_pushlstring(st, v, std::strlen(v));
		}

		// push to stack std::map
		template<typename _key, typename _val, typename _comp, typename _alloc>
		void push(const std::map<_key, _val, _comp, _alloc>& v)
		{
			lua_newtable(st);

			for (typename std::map<_key, _val, _comp, _alloc>::const_iterator i = v.begin(); i != v.end(); ++i)
			{
				push(i->first);
				push(i->second);
				lua_rawset(st, -3);
			}
		}

		// push to stack std::vector
		template<typename _val, typename _alloc>
		void push(const std::vector<_val, _alloc>& v)
		{
			lua_newtable(st);

			for (int i = 0; i < v.size(); ++i)
			{
				push(i);
				push(v[i]);
				lua_rawset(st, -3);
			}
		}


		// recv from stack scalar
		void get(unsigned int& v, int index)
		{
			v = lua_tointeger(st, index);
		}
		void get(int& v, int index)
		{
			v = lua_tointeger(st, index);
		}
		void get(bool& v, int index)
		{
			v = lua_toboolean(st, index);
		}
		void get(double& v, int index)
		{
			v = lua_tonumber(st, index);
		}
		void get(const char*& v, int index)
		{
			size_t len = 0;
			v = lua_tolstring(st, index, &len);
		}
		void get(std::string& v, int index)
		{
			size_t len = 0;

			const char* p = lua_tolstring(st, index, &len);
			if (p)
				v.assign(p, len);
		}

		// recv from stack std::map
		template<typename _key, typename _val, typename _comp, typename _alloc>
		void get(std::map<_key, _val, _comp, _alloc>& v, int index)
		{
			if (lua_type(st, index) == LUA_TTABLE)
			{
				if (index < 0)
					index = lua_gettop(st) + 1 + index;

				lua_pushnil(st);

				while (lua_next(st, index))
				{
					_val _v; get(_v, -1);
					_key _k; get(_k, -2);
					v[_k] = _v;
					pop();
				}
			}
		}

		// recv from stack std::vector (use std::map instead)
		template<typename _val, typename _alloc>
		void get(std::vector<_val, _alloc>& v, int index)
		{
			if (lua_type(st, index) == LUA_TTABLE)
			{
				if (index < 0)
					index = lua_gettop(st) + 1 + index;

				lua_pushnil(st);
				while (lua_next(st, index))
				{
					_val _v; get(_v, -1);

					if (lua_type(st, -2) == LUA_TNUMBER)
					{
						int _k = lua_tointeger(st, -2);
						if (_k >= 0)
						{
							if (_k >= v.size())
								v.resize(_k + 1);
							v[_k] = _v;
						}
					}

					pop();
				}
			}
		}


		// get value of stack with index check
		template<typename T>
		void at(int index, T& v)
		{
			if (index > 0 && index <= size())
				get(v, index);
			else
				v = T();
		}

		// pop last values from stack
		void pop()
		{
			lua_pop(st, 1);
		}
		void popn(int n)
		{
			lua_pop(st, n);
		}
	protected:
		lua_State* st;
		int st_top;
	};


	// manipulators for lua::bind

	class invoke_type {};
	class m_invoke_type {};
	class release_type {};

	extern invoke_type invoke;		// execute function
	extern m_invoke_type m_invoke;	// execute function as method (insert this pointer before argument list)
	extern release_type end;		// release lua::bind (pop staff from Lua stack)

	class lookup			// find field in last table (or LUA_GLOBALSINDEX if first)
	{
	public:
		lookup(const std::string& _name) :name(_name)
		{
		}

		friend class bind;
	protected:
		std::string name;
	};


	// class for direct access to Lua table fields

	class table : protected stack
	{
	public:
		table() : stack(nullptr), index(0)
		{
		}
		explicit table(lua_State* _st, int _index) : stack(_st), index(_index)
		{
		}
		explicit table(vm& _vm) :stack(_vm.get_st()), index(LUA_GLOBALSINDEX)
		{
		}

		// get field value
		template<typename _VAL>
		void query(const char* name, _VAL& val)
		{
			if (st)
			{
				lua_getfield(st, index, name);
				stack::get(val, -1);
				stack::pop();
			}
		}

		// set field value
		template<typename _VAL>
		void update(const char* name, const _VAL& val)
		{
			if (st)
			{
				stack::push(val);
				lua_setfield(st, index, name);
			}
		}
	protected:
		int index;
	};


	// Lua transaction class (only one per vm instance)

	class bind : protected stack
	{
	public:
		explicit bind(vm& _vm) : stack(_vm), refuse(0), args_number(0), retvals_number(0),
			cur_index(LUA_GLOBALSINDEX), mutex(_vm.mutex)
		{
			if (mutex)
				refuse++;
			else
				mutex++;
		}

		~bind()
		{
			end();
		}

		// find field in last table
		void lookup(const char* name);

		// return lua::table object for last table
		lua::table table()
		{
			if (lua_type(st, -1) == LUA_TTABLE)
				return lua::table(st, lua_gettop(st));
			return lua::table();
		}

		// execute last function
		void invoke();

		// execute last function as method
		void m_invoke();

		// end transaction (free Lua stack)
		void end();

		// push function argument to Lua stack
		template<typename T> bind& operator<<(const T& v)
		{
			if (!refuse)
			{
				push(v);
				args_number++;
			}
			return *this;
		}

		// pop function return value from stack
		template<typename T> bind& operator >> (T& v)
		{
			if (refuse || retvals_number < 1)
			{
				// INFO: (HarpyWar) -2 is default value if Lua code failed to run due to error
				//                  it's needed for lua_handle_command()
				if (typeid(T) == typeid(int))
					v = T(-2);
				else
					v = T();
			}
			else
			{
				get(v, -retvals_number); retvals_number--;
			}

			return *this;
		}

		// for manipulators
		bind& operator<<(const lua::lookup& f)
		{
			bind::lookup(f.name.c_str());
			return *this;
		}
		bind& operator<<(const lua::invoke_type&)
		{
			bind::invoke();
			return *this;
		}
		bind& operator<<(const lua::m_invoke_type&)
		{
			bind::m_invoke();
			return *this;
		}
		bind& operator<<(const lua::release_type&)
		{
			bind::end();
			return *this;
		}

	protected:
		int args_number;
		int retvals_number;
		int cur_index;

	private:
		int& mutex;
		int refuse;
	};

	using transaction = bind;
	using function = lookup;
	using field = lookup;
}


#endif
#endif