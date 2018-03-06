/* 
Copyright (C) 2013-2017 Robert DeSantis
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

#define PARSER_STACK_SIZE	100
#define MAXTAGNAMELEN       256

#ifdef COLOR_SUPPORT
#include "RGBWA.h"
#endif

// This is the simplest of JSON parsers - handles a single level of various typed objects

class SimpleJsonTokenizer
{
	LPCSTR		m_start;
	LPCSTR		m_head;

	char		m_last_char;
	char		m_break_char;
	LPCSTR		m_break_chars;
	bool		m_store_breaks;

	bool		m_in_token;
	bool		m_must_break;

	bool		m_token_available;
	bool		m_break_available;

    LPSTR		m_token;
    size_t      m_token_buf_len;

public:
	SimpleJsonTokenizer( LPCSTR data, LPCSTR break_chars, bool store_breaks ) :
		m_in_token( false ),
		m_must_break( false ),
		m_token_available( false ),
		m_break_available( false ),
		m_break_chars( break_chars ),
		m_store_breaks( store_breaks ),
		m_last_char( '\0' ),
		m_head( data ),
		m_start( data ),
        m_token( NULL ),
        m_token_buf_len(0)
	{
        m_token = (LPSTR)malloc( 256 );
        m_token_buf_len = 256;
    }

	~SimpleJsonTokenizer() {
        if ( m_token != NULL )
            free( m_token );
    }

	bool hasToken();
	LPSTR nextToken();

private:
	void advanceToken();
	void extract_token( LPCSTR start, LPCSTR end );
};

enum JsonNodeType {
    JSONROOT,
    JSONOBJECT,
    JSONARRAY,
    JSONCONSTANT,
    JSONNULL
};

class SimpleJsonParser;
class JsonNode;

typedef std::vector<JsonNode*> JsonNodePtrArray;
typedef std::vector<CString> JsonKeyArray;

class JsonNode {
	JsonNodeType        m_type;
    char				m_tagname[MAXTAGNAMELEN+1];

    char                m_value_buf[256];
    LPSTR	            m_value_ptr;

	JsonNode*			m_children;
	JsonNode*			m_next;

public:
	JsonNode() :
		m_next( NULL ),
		m_children( NULL ),
        m_value_ptr( NULL )
	{
        m_value_buf[0] = '\0';
    }

    JsonNode( JsonNodeType type, LPCSTR tagname ) :
		m_next( NULL ),
		m_children( NULL ),
        m_type( type ),
        m_value_ptr( NULL )
    {
		strncpy_s( m_tagname, tagname, sizeof(m_tagname) );
        m_value_buf[0] = '\0';
	}

    JsonNode( LPCSTR constant, LPCSTR tagname ) :
		m_next( NULL ),
		m_children( NULL ),
        m_value_ptr( NULL )
    {
        strncpy_s( m_tagname, tagname, sizeof(m_tagname) );

        if ( constant == NULL || !strcmp( constant, "null" ) ) {
            m_type = JSONNULL;
            m_value_buf[0] = '\0';
        }
        else {
            m_type = JSONCONSTANT;

            size_t len = strlen( constant );
            if ( len+1 > sizeof(m_value_buf) )
                m_value_ptr = _strdup( constant );
            else
                memcpy( m_value_buf, constant, len+1 );
        }
    }

	~JsonNode() {
		if ( m_value_ptr != NULL ) {
			free( m_value_ptr );
            m_value_ptr = NULL;
		}

		clearChildren();
	}

	JsonNode( const JsonNode& other ) {
		m_type = other.m_type;
		m_next = NULL;
        m_value_buf[0] = '\0';
        m_value_ptr = NULL;

        strncpy_s( m_tagname, other.m_tagname, sizeof(m_tagname) );

        if ( other.m_value_ptr != NULL )
            m_value_ptr = _strdup( other.m_value_ptr );
        else
            memcpy( m_value_buf, other.m_value_buf, strlen(other.m_value_buf)+1 );

		copyChildren( other );
	}

	JsonNode& operator=( const JsonNode& other ) {
		m_type = other.m_type;
		m_next = NULL;
        m_value_buf[0] = '\0';

		if ( m_value_ptr != NULL ) {
			free( m_value_ptr );
            m_value_ptr = NULL;
        }

        strncpy_s( m_tagname, other.m_tagname, sizeof(m_tagname) );

        if ( other.m_value_ptr != NULL )
            m_value_ptr = _strdup( other.m_value_ptr );
        else
            memcpy( m_value_buf, other.m_value_buf, strlen(other.m_value_buf)+1 );

		clearChildren();
		copyChildren( other );

		return *this;
	}

    inline bool isNull() {
        return m_type == JSONNULL;
    }

    inline bool isConstant() {
        return m_type == JSONCONSTANT;
    }

    inline bool isArray() {
        return m_type == JSONARRAY;
    }

	inline size_t valueCount( ) const {
		size_t count = 0;
		for ( JsonNode* node=m_children; node != NULL; node=node->m_next) 
			count++;
		return count;
	}

	inline JsonNode* setValue( LPCSTR constant, LPCSTR tagName ) {
		return addNode( new JsonNode( constant, tagName ) );
	}

	inline JsonNode* setValue( JsonNodeType type, LPCSTR tagName ) {
		return addNode( new JsonNode( type, tagName ) );
	}

    void dump();

	inline JsonNodeType getType() const {
		return m_type;
	}
	inline void setType( JsonNodeType type ) {
		m_type = type;
	}

    inline LPCSTR getTagName() const {
        return m_tagname;
    }

    inline JsonKeyArray keys() {
        JsonKeyArray keys;
		for ( JsonNode* node=m_children; node != NULL; node = node->m_next ) 
            keys.push_back( node->getTagName() );
        return keys;
    }

    inline bool has_key( LPCSTR key ) {
		return findNode( key ) != NULL;
    }

    inline bool is_null( ) {
        return m_children == NULL;			// TODO = Is this correct?
    }

    inline bool is_null( LPCSTR key ) {
		JsonNode* node = find( key );
		return ( node != NULL && node->isNull() );
    }

	bool validateArray() const {
		// Assert all non-null nodes in the array are the same type
		JsonNodeType expected_type = JSONNULL;

		for ( JsonNode* node=m_children; node != NULL; node = node->m_next ) {
			if ( node->getType() == JSONNULL )
				;
			else if ( expected_type == JSONNULL )
				expected_type = node->getType();
			else if ( node->getType() != expected_type ) {
				return false;
			}
		}
		return true;
	}

    template <class T>
    T get( LPCSTR key ) {
        T converted_value;

		find( key )->convert( converted_value );

        return converted_value;
    }

    JsonNode* getObject( LPCSTR key ) {
		return find( key );
    }

    JsonNodePtrArray getObjects( LPCSTR key ) {
		JsonNode* node = find( key );
		if ( node == NULL )
			return JsonNodePtrArray();
		return node->getObjects();
    }

    JsonNodePtrArray getObjects() {
        if ( m_type != JSONARRAY ) {
            CString error;
            error.Format( "Requested JSON node '%s' is not an object array", m_tagname );
            throw std::exception( (LPCSTR)error );
        }

        JsonNodePtrArray values;

		for ( JsonNode* node=m_children; node != NULL; node = node->m_next ) 
            values.push_back( node );

        return values;
    }

    template <class T>
    T get( LPCSTR key, T default_value ) {
        if ( !has_key( key ) )
            return default_value;

        return get<T>( key );
    }

    template <class T>
    std::vector<T> getArrayAsVector( LPCSTR key ) {
		JsonNode* container_node = find( key );

		if ( !container_node->isArray() ) {
			CString error;
			error.Format( "Requested JSON tag '%s' is not an array in node '%s'", key, m_tagname );
			throw std::exception( (LPCSTR)error );
		}

		return container_node->getArrayAsVector<T>();
    }

    template <class T>
    std::vector<T> getArrayAsVector() {
        if ( !isArray() ) {
            CString error;
            error.Format( "JSON node '%s' is not an array", m_tagname );
            throw std::exception( (LPCSTR)error );
        }

        std::vector<T> converted_array;

		for ( JsonNode* node=m_children; node != NULL; node=node->m_next ) {
            T converted_value;
            node->convert( converted_value );
            converted_array.push_back( converted_value );
        }

        return converted_array;
    }

    template <class T>
    std::set<T> getArrayAsSet( LPCSTR key ) {
		JsonNode* container_node = find( key );         

        if ( !container_node->isArray() ) {
            CString error;
            error.Format( "Requested JSON tag '%s' is not an array in node '%s'", key, m_tagname );
            throw std::exception( (LPCSTR)error );
        }

        std::set<T> converted_array;

		for ( JsonNode* node=container_node->m_children; node != NULL; node=node->m_next ) {
            T converted_value;
            node->convert( converted_value );
            converted_array.insert( converted_value );
        }

        return converted_array;
    }

    template <class T>
    T getHex( LPCSTR key ) {
		JsonNode* node = find( key );  

        if ( !node->isConstant() ) {
            CString error;
            error.Format( "Requested JSON tag '%s' is not a constant in node '%s'", key, m_tagname );
            throw std::exception( (LPCSTR)error );
        }

        T converted_value;

        node->convertHex( converted_value );

        return converted_value;
    }

private:
    JsonNode* find( LPCSTR key ) {
		JsonNode* node = findNode( key );
		if ( node != NULL )
			return node;

		CString error;
        error.Format( "Requested JSON tag '%s' is not available in node '%s'", key, m_tagname );
        throw std::exception( (LPCSTR)error );
    }

	JsonNode* findNode( LPCSTR key ) {
		char first_ch = key[0];

		for ( JsonNode* node=m_children; node != NULL; node = node->m_next ) {
			if ( first_ch != node->m_tagname[0] )
				continue;

			int cmp = strcmp( node->m_tagname, key );
			if ( cmp == 0 )
				return node;
		}
	
		return NULL;
	}

	inline JsonNode* addNode(JsonNode* node) {
		JsonNode** ptr = &m_children;

		if ( m_type == JSONARRAY ) {			// ALWAYS add to the end of an array
			while (*ptr != NULL)
				ptr = &(*ptr)->m_next;
		}
		else {
			node->m_next = *ptr;
		}
		
		*ptr = node;

		return node;
	}

    void convert( JsonNode& result ) {
        if ( m_type != JSONOBJECT ) {
            CString error;
            error.Format( "Requested node '%s' value is not a JSON object", m_tagname );
            throw std::exception( (LPCSTR)error );
        }

        result = *this;
    }

    void convert( CString& result ) {
        LPCSTR value = getValue();

		if ( value != NULL ) {
			result = value;
			result.Replace( "\\\"", "\"" );
		}
		else
			result.Empty();
    }
    
	void convert( unsigned long& result ) {
        LPCSTR value = getValue();

        if ( value == NULL || sscanf_s( value, "%lu", &result ) != 1 )
			throw_convert_error( "unsigned long" );
    }

    void convert( UINT64& result ) {
        LPCSTR value = getValue();

        if ( value == NULL || sscanf_s( value, "%llu", &result ) != 1 )
            throw_convert_error( "unsigned long long" );
    }

    void convert( unsigned& result ) {
        LPCSTR value = getValue();

        if ( value == NULL || sscanf_s( value, "%u", &result ) != 1 )
			throw_convert_error( "unsigned" );
    }

    void convert( long& result ) {
        LPCSTR value = getValue();

        if ( value == NULL || sscanf_s( value, "%ld", &result ) != 1 )
			throw_convert_error( "long" );
    }

    void convert( int& result ) {
        LPCSTR value = getValue();

        if ( value == NULL || sscanf_s( value, "%d", &result ) != 1 )
			throw_convert_error( "int" );
    }

    void convert( WORD& result ) {
        LPCSTR value = getValue();

        if ( value == NULL || sscanf_s( value, "%hu", &result ) != 1 )
			throw_convert_error( "WORD" );
    }

    void convert( BYTE& result ) {
        LPCSTR value = getValue();

		if ( value == NULL )
			throw_convert_error( "BYTE" );

        result = (BYTE)atoi( value );
    }

    void convert( bool& result ) {
        LPCSTR value = getValue();

		if ( value == NULL )
			throw_convert_error( "bool" );

        result = !( !strcmp( value, "0" ) || !_strcmpi( value, "false" ) );
    }

    void convert( float& result ) {
        LPCSTR value = getValue();

        if ( value == NULL || sscanf_s( value, "%f", &result ) != 1 )
			throw_convert_error( "float" );
    }

    void convert( double& result ) {
        LPCSTR value = getValue();

        if ( value == NULL || sscanf_s( value, "%lf", &result ) != 1 )
			throw_convert_error( "double" );
    }

    template <class T>
    void convert( std::vector<T>& result ) {
		for ( JsonNode* node=m_children; node != NULL; node=node->m_next ) {
            T lvalue;
            node->convert( lvalue );
            result.push_back( lvalue );
        }
    }

    template <class T>
    void convert( std::set<T>& result ) {
		for ( JsonNode* node=m_children; node != NULL; node=node->m_next ) {
            T lvalue;
            node->convert( lvalue );
            result.insert( lvalue );
        }
    }

    template <class T>
    void convertHex( std::vector<T>& result ) {
		for ( JsonNode* node=m_children; node != NULL; node=node->m_next ) {
            T lvalue;
            node->convertHex( lvalue );
            result.push_back( lvalue );
        }
    }

    void convertHex( unsigned long& result ) {
        LPCSTR value = getValue();

        if ( value == NULL || sscanf_s( value, "%lx", &result ) != 1 )
			throw_convert_error( "hex value" );
    }

#ifdef COLOR_SUPPORT
    void convert( RGBWA& result ) {
        ULONG rgbwa;
        LPCSTR value = getValue();

        if ( value && value[0] == '#' )
            value++;

		if ( value == NULL || sscanf_s( value, "%lx", &rgbwa ) != 1 )
			throw_convert_error( "hex value" );

        result = RGBWA(rgbwa);
    }
#endif

	void throw_convert_error( LPCSTR expectedType ) {
        LPCSTR value = getValue();

		CString error;
		error.Format( "Node '%s' value '%s' is not an %s", 
			m_tagname, value == NULL ? "NULL" : value, expectedType );
		throw std::exception( (LPCSTR)error );
	}

    inline LPCSTR getValue() const {
        return ( m_value_ptr != NULL ) ? m_value_ptr : m_value_buf;
    }

protected:
	void copyChildren( const JsonNode& other ) {
		JsonNode** ptr = &m_children;
		*ptr = NULL;

		for ( JsonNode* child=other.m_children; child != NULL; child = child->m_next ) {
			JsonNode* node = new JsonNode( *child );
			*ptr = node;
			ptr = &node->m_next;
		}
	}

	void clearChildren() {
		for ( JsonNode* node=m_children; node != NULL; ) {
			JsonNode* next = node->m_next;
			delete node;
			node = next;
		}

		m_children = NULL;
	}
};

class SimpleJsonParser : public JsonNode
{
	JsonNode*		m_nodeStack[PARSER_STACK_SIZE];	
	unsigned		m_stack_ptr;

public:
    SimpleJsonParser(void);
    ~SimpleJsonParser(void);

    void parse( LPCSTR json_data );
    void parse( FILE* fp );
	void parse( SimpleJsonTokenizer& st );

	inline void reset( ) {
		clearChildren();
		setType( JSONROOT );
		m_stack_ptr = 0;
	}

private:
    LPSTR strip_quotes( LPSTR value );
	
	inline void push( JsonNode* node ) {
		if ( m_stack_ptr == PARSER_STACK_SIZE )
			throw std::exception( "JSON parser stack overflow - increase stack size" );
		m_nodeStack[m_stack_ptr++] = node;
	}

	inline JsonNode* pop( ) {
		if ( m_stack_ptr == 0 )
			throw std::exception( "JSON parser stack underflow" );

		return m_nodeStack[--m_stack_ptr]; 
	}

	inline JsonNode* top( ) const {
		if ( m_stack_ptr == 0 )
			throw std::exception( "JSON parser stack underflow" );

		return m_nodeStack[m_stack_ptr-1]; 
	}

	inline size_t stackSize( ) const {
		return m_stack_ptr;
	}
};

