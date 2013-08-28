/*
  This file is provided under a dual BSD/GPLv2 license.  When using or
  redistributing this file, you may do so under either license.

  GPL LICENSE SUMMARY

  Copyright(c) 2005-2009 Intel Corporation. All rights reserved.

  This program is free software; you can redistribute it and/or modify
  it under the terms of version 2 of the GNU General Public License as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
  The full GNU General Public License is included in this distribution
  in the file called LICENSE.GPL.

  Contact Information:
   Intel Corporation

   2200 Mission College Blvd.
   Santa Clara, CA  97052

  BSD LICENSE 

  Copyright(c) 2005-2009 Intel Corporation. All rights reserved.
  All rights reserved.

  Redistribution and use in source and binary forms, with or without 
  modification, are permitted provided that the following conditions 
  are met:

    * Redistributions of source code must retain the above copyright 
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright 
      notice, this list of conditions and the following disclaimer in 
      the documentation and/or other materials provided with the 
      distribution.
    * Neither the name of Intel Corporation nor the names of its 
      contributors may be used to endorse or promote products derived 
      from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 */
#ifndef _OSAL_CHAR_LINUX_USER_H
#define _OSAL_CHAR_LINUX_USER_H
#ifdef _UNICODE
#include <wchar.h>
#define TCHAR wchar_t
#include <string.h>
#include <strings.h>
#include <stdarg.h>
#include <stdio.h>

#define _tprintf   wprintf
#define _ftprintf fwprintf
#define _stprintf swprintf
#define _vtprintf vwprintf
#define _vftprintf vfwprintf
#define _vstprintf vswprintf
#define _tcscmp wcscmp
#define _tcstol wcstol
#define _tcslen wcslen
#define _tcscat wcscat
#define _tcsncat wcsncat
#define _tcsncpy wcsncpy
#define _tcspbrk wcspbrk
#define _tcsrchr wcsrchr
#define _tcsspn wcsspn
#define _tcsstr wcsstr
#define _tcstok wcstok
#define _tcschr wcschr
#define _tcsupr(x) x
#define _tcscpy wcscpy
#define _tcscspn wcscspn
#define _tcsdup wcsdup
#define _tcsnccmp wcsncmp
#define _tcsncmp wcsncmp
#define _tstof _wtof
#define _tstol _wtol
#define _tstoi _wtoi
#define _fgettc fgetwc
#define _fgetts fgetws
#define _fputtc fputwc
#define _fputts fputws
#define _gettc getwc
#define _gettchar getwchar
#define _puttc putwc
#define _puttchar putwchar
#define _ungettc ungetwc

#define _T(x) Lx

#define exit return 
#define _tcscmp wcscmp
#define _tcsncmp wcsncmp
#define _ftprintf fwprintf
#define _tcsicmp wcscasecmp
#define _tcsincmp wcsncasecmp
#define _stprintf swprintf


#define _istalnum iswalnum
#define _istalpha iswalpha
#define _istcntrl iswcntrl
#define _istdigit iswdigit
#define _istgraph iswgraph
#define _istlower iswlower
#define _istprint iswprint
#define _istpunct iswpunct
#define _istspace iswspace
#define _istupper iswupper
#define _istxdigit iswxdigit

#define _totupper towupper
#define _totlower towlower


#else

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>

#define TCHAR char
#define _tprintf   printf
#define _ftprintf fprintf
#define _stprintf sprintf
#define _vtprintf vprintf
#define _vftprintf vfprintf
#define _vstprintf vsprintf

#define _tcscmp strcmp
#define _tcstol strtol
#define _tcslen strlen
#define _tcscat strcat
#define _tcsncat strncat
#define _tcsncpy strncpy
#define _tcspbrk strpbrk
#define _tcsrchr strrchr
#define _tcsspn strspn
#define _tcsstr strstr
#define _tcstok strtok
#define _tcsdup strdup
#define _tcsnccmp strncmp
#define _tcsncmp strncmp
#define _tcschr strchr
#define _tfopen fopen
#define _tcscpy strcpy
#define _tcsupr(x) x
#define _tcscspn strcspn
#define _tstof atof
#define _tstol atol
#define _tstoi atoi

#define _tmain main
#define _T(x) x


#define _tcscmp strcmp
#define _ftprintf fprintf
#define _tcsicmp strcasecmp
#define _tcsnicmp strncasecmp
#define _stprintf sprintf


#define _fgettc fgetc
#define _fgetts fgets
#define _fputtc fputc
#define _fputts fputs
#define _gettc getc
#define _gettchar getchar
#define _puttc putc
#define _puttchar putchar
#define _ungettc ungetc

#define _istalnum isalnum
#define _istalpha isalpha
#define _istcntrl iscntrl
#define _istdigit isdigit
#define _istgraph isgraph
#define _istlower islower
#define _istprint isprint
#define _istpunct ispunct
#define _istspace isspace
#define _istupper isupper
#define _istxdigit isxdigit

#define _totupper toupper
#define _totlower tolower

#endif
#endif
