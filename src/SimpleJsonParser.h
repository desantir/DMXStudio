/* 
Copyright (C) 2013-2016 Robert DeSantis
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
#include "AbstractAnimation.h"

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

    bool is_null( ) {
        return m_values.size() == 0;
    }

    bool is_null( LPCSTR key ) {
        return findPair( key )->second == "null";
    }

    template <class T>
    T get( LPCSTR key ) {
        LPCSTR value = (LPCSTR)findPair( key )->second;

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
            CString& value = strip_quotes(*it);
            T converted_value;

            if ( value[0] == '{' || value [0] == '[' ) {
                parser.parse( value );
                converted_value = parser.get<T>( "" );
            }
            else        // Assume scalar values
                convert( value, converted_value );

            converted_array.push_back( converted_value );
        }

        return converted_array;
    }

    template <class T>
    T getHex( LPCSTR key ) {
        LPCSTR value = (LPCSTR)findPair( key )->second;

        T converted_value;

        convertHex( value, converted_value );

        return converted_value;
    }

    void dump() {
        for ( NAME_VALUE_PAIR::iterator it = m_values.begin(); it != m_values.end(); ++it )
            printf( "%s = %s\n", (LPCSTR)it->first, (LPCSTR)it->second );
    }
    
private:
    NAME_VALUE_PAIR::iterator findPair( LPCSTR key ) {
        NAME_VALUE_PAIR::iterator it = m_values.find( key );
        if ( it == m_values.end() ) {
            CString error;
            error.Format( "Requested JSON tag '%s' is not available", key );
            throw std::exception( (LPCSTR)error );
        }
        return it;
    }
    
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
        if ( sscanf_s( value, "%lu", &result ) != 1 ) {
            CString error;
            error.Format( "Value '%s' is not an unsigned long", value );
            throw std::exception( (LPCSTR)error );
        }
    }

    void convert( LPCSTR value, unsigned& result ) {
        if ( sscanf_s( value, "%u", &result ) != 1 ) {
            CString error;
            error.Format( "Value '%s' is not an unsigned", value );
            throw std::exception( (LPCSTR)error );
        }
    }

    void convert( LPCSTR value, long& result ) {
        if ( sscanf_s( value, "%ld", &result ) != 1 ) {
            CString error;
            error.Format( "Value '%s' is not a long", value );
            throw std::exception( (LPCSTR)error );
        }
    }

    void convert( LPCSTR value, int& result ) {
        if ( sscanf_s( value, "%d", &result ) != 1 ) {
            CString error;
            error.Format( "Value '%s' is not an int", value );
            throw std::exception( (LPCSTR)error );
        }
    }

    void convert( LPCSTR value, WORD& result ) {
        if ( sscanf_s( value, "%hu", &result ) != 1 ) {
            CString error;
            error.Format( "Value '%s' is not a WORD", value );
            throw std::exception( (LPCSTR)error );
        }
    }

    void convert( LPCSTR value, BYTE& result ) {
        result = (BYTE)atoi( value );
    }

    void convert( LPCSTR value, bool& result ) {
        result = !( !strcmp( value, "0" ) || !_strcmpi( value, "false" ) );
    }

    void convert( LPCSTR value, float& result ) {
        if ( sscanf_s( value, "%f", &result ) != 1 ) {
            CString error;
            error.Format( "Value '%s' is not a float", value );
            throw std::exception( (LPCSTR)error );
        }
    }

    void convert( LPCSTR value, double& result ) {
        if ( sscanf_s( value, "%lf", &result ) != 1 ) {
            CString error;
            error.Format( "Value '%s' is not a double", value );
            throw std::exception( (LPCSTR)error );
        }
    }

    template <class T>
    void convert( LPCSTR value, std::vector<T>& result ) {
        std::vector<CString> tokens = tokenize( value, "[],", false );
        for ( std::vector<CString>::iterator it=tokens.begin(); it != tokens.end(); ++it ) {
            T lvalue;
            convert( (LPCSTR)(*it), lvalue );
            result.push_back( lvalue );
        }
    }

    template <class T>
    void convert( LPCSTR value, std::set<T>& result ) {
        std::vector<CString> tokens = tokenize( value, "[],", false );
        for ( std::vector<CString>::iterator it=tokens.begin(); it != tokens.end(); ++it ) {
            T lvalue;
            convert( (LPCSTR)(*it), lvalue );
            result.insert( lvalue );
        }
    }

    void convertHex( LPCSTR value, unsigned long& result ) {
        if ( sscanf_s( value, "%lx", &result ) != 1 ) {
            CString error;
            error.Format( "Value '%s' is not a hex value", value );
            throw std::exception( (LPCSTR)error );
        };
    }

    void convertHex( LPCSTR value, RGBWA& result ) {
        ULONG rgbwa;
        if ( value && value[0] == '#' )
            value++;
        if ( sscanf_s( value, "%lx", &rgbwa ) != 1 ) {
            CString error;
            error.Format( "Value '%s' is not a hex value", value );
            throw std::exception( (LPCSTR)error );
        }
        result = RGBWA(rgbwa);
    }

    template <class T>
    void convertHex( LPCSTR value, std::vector<T>& result ) {
        std::vector<CString> tokens = tokenize( value, "[],", false );
        for ( std::vector<CString>::iterator it=tokens.begin(); it != tokens.end(); ++it ) {
            T lvalue;
            convertHex( (LPCSTR)(*it), lvalue );
            result.push_back( lvalue );
        }
    }
};

typedef std::vector<SimpleJsonParser> PARSER_LIST;

