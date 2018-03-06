/* 
Copyright (C) 2011,2012 Robert DeSantis
hopluvr at gmail dot com

This file is part of DMX Studio.

DMX Studio is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or (at your
option) any later version.

DMX Studio is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
License for more details.

You should have received a copy of the GNU General Public License
along with DMX Studio; see the file _COPYING.txt.  If not, write to
the Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
MA 02111-1307, USA.
*/

#pragma once

#include "stdafx.h"

#include <Winhttp.h>
#include <Shlwapi.h>

extern BOOL readBuffer( HINTERNET hRequest, BYTE **buffer, ULONG * buffer_size );
extern CString encodeString( LPCSTR source );
extern CString decodeString( LPCSTR source );
extern CString unencodeString( LPCSTR source );
extern DWORD httpGet( LPCSTR URL, BYTE **buffer, ULONG * buffer_size );
extern DWORD httpGet( LPCWSTR server_name, LPCSTR url, LPCWSTR headers, BYTE **buffer, ULONG * buffer_size );
extern size_t parseQuery( std::map<CString,CString>& parameters, LPCSTR raw_query );
extern DWORD httpPost( LPCWSTR server_name, LPCSTR url, CString& body, LPCWSTR headers, BYTE **buffer, ULONG * buffer_size );
extern BOOL encodeBase64( LPCSTR source, LPSTR target, LPINT target_len );
