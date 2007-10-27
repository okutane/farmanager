#ifndef __BaseFileMask_HPP
#define __BaseFileMask_HPP
/*
BaseFileMask.hpp

����������� �����, ������� ��� �������� ������ � �������.
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

extern const int EXCLUDEMASKSEPARATOR;


class BaseFileMask
{
  public:
    BaseFileMask() {}
    virtual ~BaseFileMask() {}

  public:
    virtual BOOL Set(const wchar_t *Masks, DWORD Flags)=0;
    virtual BOOL Compare(const wchar_t *Name)=0;
    virtual BOOL IsEmpty(void) { return TRUE; }

  private:
    BaseFileMask& operator=(const BaseFileMask& rhs); /* ����� �� */
    BaseFileMask(const BaseFileMask& rhs); /* �������������� �� ��������� */

};


#endif // __BaseFileMask_HPP
