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

#ifdef COLOR_SUPPORT
#include "RGBWA.h"
#endif

// This is the simplest of JSON parsers - handles a single level of various typed objects

enum JsonNodeType {
    JSONROOT,
    JSONOBJECT,
    JSONARRAY,
    JSONCONSTANT,
    JSONNULL
};

class SimpleJsonParser;
struct JsonNode;;

typedef std::vector<JsonNode*> JsonNodePtrArray;

struct JsonNode {
    typedef std::map< CString, JsonNode > NAME_VALUE_PAIR;

    JsonNodeType        m_type;
    CString             m_constant;
    NAME_VALUE_PAIR     m_values;

    JsonNode() {}
    ~JsonNode() {}

    JsonNode( JsonNodeType type ) :
        m_type( type )
    {}

    JsonNode( LPCSTR constant )
    {
        if ( constant == NULL || !strcmp( constant, "null" ) )
            m_type = JSONNULL;
        else {
            m_type = JSONCONSTANT;
            m_constant = constant;
        }
    }

    inline bool isNull() {
        return m_type == JSONNULL;
    }

    inline bool isConstant() {
        return m_type == JSONARRAY;
    }

    inline bool isArray() {
        return m_type == JSONCONSTANT;
    }

    void dump();

    inline bool has_key( LPCSTR key ) {
        NAME_VALUE_PAIR::iterator it = m_values.find( key );
        return it != m_values.end();
    }

    inline bool is_null( ) {
        return m_values.size() == 0;
    }

    inline bool is_null( LPCSTR key ) {
        return findPair( key )->second.isNull();
    }

    template <class T>
    T get( LPCSTR key ) {
        JsonNode& node = findPair( key )->second;

        T converted_value;

        convert( node, converted_value );

        return converted_value;
    }

    JsonNodePtrArray getObjects( LPCSTR key ) {
        return findPair( key )->second.getObjects();
    }

    JsonNodePtrArray getObjects() {
        if ( m_type != JSONARRAY ) {
            CString error;
            error.Format( "Requested JSON node is not an object array" );
            throw std::exception( (LPCSTR)error );
        }

        JsonNodePtrArray values;

        for ( NAME_VALUE_PAIR::value_type& child : m_values )
            values.push_back( &child.second );

        return values;
    }

    template <class T>
    T get( LPCSTR key, T default_value ) {
        if ( !has_key( key ) )
            return default_value;

        return get<T>( key );
    }

    template <class T>
    std::vector<T> getArray( LPCSTR key ) {
        JsonNode& node = findPair( key )->second;        

        if ( !node.isArray() ) {
            CString error;
            error.Format( "Requested JSON tag '%s' is not an array", key );
            throw std::exception( (LPCSTR)error );
        }

        std::vector<T> converted_array;

        for ( auto child : node.m_values ) {
            T converted_value;
            convert( child.second, converted_value );
            converted_array.push_back( converted_value );
        }

        return converted_array;
    }

    template <class T>
    T getHex( LPCSTR key ) {
        JsonNode& node = findPair( key )->second;        

        if ( !node.isConstant() ) {
            CString error;
            error.Format( "Requested JSON tag '%s' is not a constant", key );
            throw std::exception( (LPCSTR)error );
        }

        T converted_value;

        convertHex( node, converted_value );

        return converted_value;
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

    void convert( JsonNode& node, JsonNode& result ) {
        if ( node.m_type != JSONOBJECT )
            throw std::exception( "Requested value is not a JSON object" );

        result = node;
    }

    void convert( JsonNode& node, CString& result ) {
        result = node.m_constant;
        result.Replace( "\\\"", "\"" );
    }

    void convert( JsonNode& node, unsigned long& result ) {
        if ( sscanf_s( node.m_constant, "%lu", &result ) != 1 ) {
            CString error;
            error.Format( "Value '%s' is not an unsigned long", node.m_constant );
            throw std::exception( (LPCSTR)error );
        }
    }

    void convert( JsonNode& node, unsigned& result ) {
        if ( sscanf_s( node.m_constant, "%u", &result ) != 1 ) {
            CString error;
            error.Format( "Value '%s' is not an unsigned", node.m_constant );
            throw std::exception( (LPCSTR)error );
        }
    }

    void convert( JsonNode& node, long& result ) {
        if ( sscanf_s( node.m_constant, "%ld", &result ) != 1 ) {
            CString error;
            error.Format( "Value '%s' is not a long", node.m_constant );
            throw std::exception( (LPCSTR)error );
        }
    }

    void convert( JsonNode& node, int& result ) {
        if ( sscanf_s( node.m_constant, "%d", &result ) != 1 ) {
            CString error;
            error.Format( "Value '%s' is not an int", node.m_constant );
            throw std::exception( (LPCSTR)error );
        }
    }

    void convert( JsonNode& node, WORD& result ) {
        if ( sscanf_s( node.m_constant, "%hu", &result ) != 1 ) {
            CString error;
            error.Format( "Value '%s' is not a WORD", node.m_constant );
            throw std::exception( (LPCSTR)error );
        }
    }

    void convert( JsonNode& node, BYTE& result ) {
        result = (BYTE)atoi( node.m_constant );
    }

    void convert( JsonNode& node, bool& result ) {
        result = !( !strcmp( node.m_constant, "0" ) || !_strcmpi( node.m_constant, "false" ) );
    }

    void convert( JsonNode& node, float& result ) {
        if ( sscanf_s( node.m_constant, "%f", &result ) != 1 ) {
            CString error;
            error.Format( "Value '%s' is not a float", node.m_constant );
            throw std::exception( (LPCSTR)error );
        }
    }

    void convert( JsonNode& node, double& result ) {
        if ( sscanf_s( node.m_constant, "%lf", &result ) != 1 ) {
            CString error;
            error.Format( "Value '%s' is not a double", node.m_constant );
            throw std::exception( (LPCSTR)error );
        }
    }

    template <class T>
    void convert( JsonNode& node, std::vector<T>& result ) {
        for ( NAME_VALUE_PAIR::value_type& pair : node.m_values ) {
            T lvalue;
            convert( pair.second, lvalue );
            result.push_back( lvalue );
        }
    }

    template <class T>
    void convert( JsonNode& node, std::set<T>& result ) {
        for ( NAME_VALUE_PAIR::value_type& pair : node.m_values ) {
            T lvalue;
            convert( pair.second, lvalue );
            result.insert( lvalue );
        }
    }

    template <class T>
    void convertHex( JsonNode& node, std::vector<T>& result ) {
        for ( NAME_VALUE_PAIR::value_type& pair : node.m_values ) {
            T lvalue;
            convertHex( pair.second, lvalue );
            result.push_back( lvalue );
        }
    }

    void convertHex( JsonNode& node, unsigned long& result ) {
        if ( sscanf_s( node.m_constant, "%lx", &result ) != 1 ) {
            CString error;
            error.Format( "Value '%s' is not a hex value", node.m_constant );
            throw std::exception( (LPCSTR)error );
        };
    }

#ifdef COLOR_SUPPORT
    void convertHex( JsonNode& node, RGBWA& result ) {
        ULONG rgbwa;
        LPCSTR value = node.m_constant;

        if ( value && value[0] == '#' )
            value++;
        if ( sscanf_s( value, "%lx", &rgbwa ) != 1 ) {
            CString error;
            error.Format( "Value '%s' is not a hex value", value );
            throw std::exception( (LPCSTR)error );
        }
        result = RGBWA(rgbwa);
    }
#endif
};

class SimpleJsonParser : public JsonNode
{
public:
    SimpleJsonParser(void);
    ~SimpleJsonParser(void);

    void parse( LPCSTR json_data );
    void parse( FILE* fp );

    void parse_old( std::vector<CString>& tokens );

private:
    void parse( std::vector<CString>& tokens );
    std::vector<CString> tokenize( LPCSTR value, LPCSTR break_chars=",",  bool store_breaks=false );
    CString& strip_quotes( CString& value );
};

