/*
new.cpp

������ RTL-�����
*/
/*
Copyright (c) 1996 Eugene Roshal
Copyright (c) 2000 Far Group
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

#include "headers.hpp"
#pragma hdrstop

extern "C" {
void *__cdecl xf_malloc(size_t __size);
};

#if defined(SYSLOG)
extern long CallNewDelete;
#endif


#if defined(_MSC_VER)
extern _PNH _pnhHeap;

extern "C" {
void * __cdecl  _nh_malloc(size_t, int);
};

void * operator new( size_t cb )
{
  // ����� ��� - �� ������ - �� (�᫨ �� ����� - ᤥ����!!!)
  void *res = xf_malloc(cb);//_nh_malloc( cb, 1 );
#if defined(SYSLOG)
  CallNewDelete++;
#endif
  return res;
}

#elif defined(__BORLANDC__)
extern new_handler _new_handler;

//namespace std {

new_handler set_new_handler(new_handler p)
{
  new_handler t = _new_handler;
  _new_handler = p;
  return t;
}
//}

void *operator new(size_t size)
{
  void * p;

  size = size ? size : 1;

  /* FDIS 18.4.1.1 (3,4) now require new to throw bad_alloc if the
     most recent call to set_new_handler was passed NULL.
     To ensure no exception throwing, use the forms of new that take a
     nothrow_t, as they will call straight to malloc().
  */
  while ( (p = xf_malloc(size)) == NULL)
      if (_new_handler)
          _new_handler();
      else
         /* This is illegal according to ANSI, but if we've compiled the
            RTL without exception support we had better just return NULL.
         */
         break;
#if defined(SYSLOG)
  CallNewDelete++;
#endif
  return p;
}

#endif
