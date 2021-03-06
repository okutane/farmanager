﻿#ifndef SMART_PTR_HPP_DE65D1E8_C925_40F7_905A_B7E3FF40B486
#define SMART_PTR_HPP_DE65D1E8_C925_40F7_905A_B7E3FF40B486
#pragma once

/*
smart_ptr.hpp
*/
/*
Copyright © 2014 Far Group
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the authors may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

template<typename T>
class array_ptr: public conditional<array_ptr<T>>
{
public:
	NONCOPYABLE(array_ptr);
	MOVABLE(array_ptr);

	array_ptr() noexcept: m_size() {}
	array_ptr(size_t size, bool init = false) { reset(size, init); }

	void reset(size_t size, bool init = false) { m_array.reset(init? new T[size]() : new T[size]); m_size = size;}
	void reset() noexcept { m_array.reset(); m_size = 0; }
	size_t size() const noexcept { return m_size; }
	bool operator!() const noexcept { return !m_array; }
	decltype(auto) get() const noexcept {return m_array.get();}
	decltype(auto) operator[](size_t n) const { assert(n < m_size); return m_array[n]; }

private:
	std::unique_ptr<T[]> m_array;
	size_t m_size;
};

using wchar_t_ptr = array_ptr<wchar_t>;
using char_ptr = array_ptr<char>;

template<typename T>
class block_ptr: public char_ptr
{
public:
	NONCOPYABLE(block_ptr);
	MOVABLE(block_ptr);

	using char_ptr::char_ptr;
	block_ptr() = default;
	decltype(auto) get() const noexcept {return reinterpret_cast<T*>(char_ptr::get());}
	decltype(auto) operator->() const noexcept { return get(); }
	decltype(auto) operator*() const noexcept {return *get();}
};

template <typename T>
class unique_ptr_with_ondestroy: public conditional<unique_ptr_with_ondestroy<T>>
{
public:
	~unique_ptr_with_ondestroy() { OnDestroy(); }
	decltype(auto) get() const noexcept { return ptr.get(); }
	decltype(auto) operator->() const noexcept { return ptr.operator->(); }
	decltype(auto) operator*() const { return *ptr; }
	bool operator!() const noexcept { return !ptr; }
	decltype(auto) operator=(std::unique_ptr<T>&& value) noexcept { OnDestroy(); ptr = std::move(value); return *this; }

private:
	void OnDestroy() { if (ptr) ptr->OnDestroy(); }

	std::unique_ptr<T> ptr;
};

namespace detail
{
	struct file_closer
	{
		void operator()(FILE* Object) const
		{
			fclose(Object);
		}
	};
}

using file_ptr = std::unique_ptr<FILE, detail::file_closer>;

template<typename T>
class ptr_setter_t
{
public:
	NONCOPYABLE(ptr_setter_t)
	MOVABLE(ptr_setter_t)
	ptr_setter_t(T& Ptr): m_Ptr(&Ptr) {}
	~ptr_setter_t() { if (m_Ptr) m_Ptr->reset(m_RawPtr); }
	auto operator&() { return &m_RawPtr; }

private:
	movalbe_ptr<T> m_Ptr;
	typename T::pointer m_RawPtr{};
};

template<typename T>
auto ptr_setter(T& Ptr) { return ptr_setter_t<T>(Ptr); }

#endif // SMART_PTR_HPP_DE65D1E8_C925_40F7_905A_B7E3FF40B486
