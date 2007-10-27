#include "crt.hpp"

_CONST_RETURN void * __cdecl memchr(const void *buf, int chr, size_t cnt)
{
  while (cnt && (*(unsigned char *)buf != (unsigned char)chr))
  {
    buf = (unsigned char *)buf + 1;
    cnt--;
  }
  return(cnt ? (void *)buf : NULL);
}

//---------------------------------------------------------------------------
_CONST_RETURN wchar_t * __cdecl wmemchr(const wchar_t *buf, int chr, size_t cnt)
{
  while (cnt && (*buf != (wchar_t)chr))
  {
    buf++;
    cnt--;
  }
  return(cnt ? (wchar_t *)buf : NULL);
}

//---------------------------------------------------------------------------
