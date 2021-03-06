﻿#ifndef FUNCTION_TRAITS_HPP_071DD1DD_F933_40DC_A662_CB85F7BE7F00
#define FUNCTION_TRAITS_HPP_071DD1DD_F933_40DC_A662_CB85F7BE7F00
#pragma once

/*
function_traits.hpp
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

namespace detail
{
	template<typename T>
	using root_type_t = std::remove_cv_t<std::remove_reference_t<std::remove_pointer_t<T>>>;

	template<typename> struct function_traits;

	template<typename result, typename... args>
	struct function_traits_impl
	{
		using return_type_t = root_type_t<result>;
	};

	template<typename result, typename... args>
	struct function_traits<result(*)(args...)>: function_traits_impl<result, args...> {};

	template<typename result, typename object, typename... args>
	struct function_traits<result(object::*)(args...)>: function_traits_impl<result, args...> {};

	template<typename result, typename object, typename... args>
	struct function_traits<result(object::*)(args...) const>: function_traits_impl<result, args...> {};
}

template<typename function>
using return_type_t = typename detail::function_traits<function>::return_type_t;

#define FN_RETURN_TYPE(function_name) return_type_t<decltype(&function_name)>

#endif // FUNCTION_TRAITS_HPP_071DD1DD_F933_40DC_A662_CB85F7BE7F00
