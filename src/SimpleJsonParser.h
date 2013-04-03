/* 
Copyright (C) 2013 Robert DeSantis
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

// This is the simplest of JSON parsers - handles a single level of various typed objects

class SimpleJsonParser
{
    typedef std::map< CString, CString > NAME_VALUE_PAIR;

    enum ParseState {
        OPENING_BRACE=1,
        TAG_NAME,
        COLON,
        TAG_VALUE,
        TAG_SEPARATOR,
        ARRAY_VALUE_OR_SEPARATOR,
        ARRAY_VALUE,
        ARRAY_SEPARATOR,
        ARRAY_OF_OBJECTS,
        OBJECT,
        DONE
     };
          
    NAME_VALUE_PAIR         m_values;

public:
    SimpleJsonParser(void);
    ~SimpleJsonParser(void);

    void parse( LPCSTR json_data );

    bool has_key( LPCSTR key ) {
        NAME_VALUE_PAIR::iterator it = m_values.find( key );
        return it != m_values.end();
    }

    template <class T>
    T get( LPCSTR key ) {
        NAME_VALUE_PAIR::iterator it = m_values.find( key );
        if ( it == m_values.end() ) {
            CString error;
            error.Format( "Requested JSON tag '%s' is not available", key );
            throw std::exception( (LPCSTR)error );
        }

        LPCSTR value = (LPCSTR)it->second;

        T converted_value;

        convert( value, converted_value );

        return converted_value;
    }

    template <class T>
    std::vector<T> getArray( LPCSTR key ) {
        std::vector<T> converted_array;

        // Array items will be quote delimited
        std::vector<CString> array_items = get<std::vector<CString>>(key);
        SimpleJsonParser parser;

        for ( std::vector<CString>::iterator it=array_items.begin(); it != array_items.end(); ++it ) {
            parser.parse( strip_quotes(*it) );
            T converted_value = parser.get<T>( "" );
            converted_array.push_back( converted_value );
        }
         
        return converted_array;
    }

    template <class T>
    T getHex( LPCSTR key ) {
        NAME_VALUE_PAIR::iterator it = m_values.find( key );
        if ( it == m_values.end() ) {
            CString error;
            error.Format( "Requested JSON tag '%s' is not available", key );
            throw std::exception( (LPCSTR)error );
        }

        LPCSTR value = (LPCSTR)it->second;

        T converted_value;

        convertHex( value, converted_value );

        return converted_value;
    }

    void dump() {
        for ( NAME_VALUE_PAIR::iterator it = m_values.begin(); it != m_values.end(); ++it )
            printf( "%s = %s\n", it->first, it->second );
    }
    
private:    
    std::vector<CString> tokenize( LPCSTR value, LPCSTR break_chars=",",  bool store_breaks=false );
    CString&  SimpleJsonParser::strip_quotes( CString& value );

    void convert( LPCSTR value, SimpleJsonParser& parser ) {
        CString unescaped( value );
        unescaped.Replace( "\\\\", "\\" );
        unescaped.Replace( "\\\"", "\"" );
        strip_quotes( unescaped );
        parser.parse( unescaped );
    }

    void convert( LPCSTR value, CString& result ) {
        result = value;
        result.Replace( "\\\"", "\"" );
    }

    void convert( LPCSTR value, unsigned long& result ) {
        sscanf_s( value, "%lu", &result );
    }

    void convert( LPCSTR value, unsigned& result ) {
        sscanf_s( value, "%u", &result );
    }

    void convert( LPCSTR value, long& result ) {
        result = atol( value );
    }

    void convert( LPCSTR value, int& result ) {
        result = atoi( value );
    }

    void convert( LPCSTR value, WORD& result ) {
        result = atoi( value );
    }

    void convert( LPCSTR value, BYTE& result ) {
        result = (BYTE)atoi( value );
    }

    void convert( LPCSTR value, bool& result ) {
        result = !( !strcmp( value, "0" ) || !_strcmpi( value, "false" ) );
    }

    void convert( LPCSTR value, float& result ) {
        result = (float)atof( value );
    }

    void convert( LPCSTR value, double& result ) {
        result = atof( value );
    }

    template <class T>
    void convert( LPCSTR value, std::vector<T>& result ) {
        std::vector<CString> tokens = tokenize( value );
        for ( std::vector<CString>::iterator it=tokens.begin(); it != tokens.end(); ++it ) {
            T lvalue;
            convert( (LPCSTR)(*it), lvalue );
            result.push_back( lvalue );
        }
    }

    void convertHex( LPCSTR value, unsigned long& result ) {
        sscanf_s( value, "%lX", &result );
    }

    template <class T>
    void convertHex( LPCSTR value, std::vector<T>& result ) {
        std::vector<CString> tokens = tokenize( value );
        for ( std::vector<CString>::iterator it=tokens.begin(); it != tokens.end(); ++it ) {
            T lvalue;
            convertHex( (LPCSTR)(*it), lvalue );
            result.push_back( lvalue );
        }
    }
};

typedef std::vector<SimpleJsonParser> PARSER_LIST;

