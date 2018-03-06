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

#include "SimpleJsonParser.h"

// ----------------------------------------------------------------------------
//
SimpleJsonParser::SimpleJsonParser(void) :
    JsonNode( JsonNodeType::JSONROOT, "<ROOT>" ),
	m_stack_ptr( 0 )
{
}

// ----------------------------------------------------------------------------
//
SimpleJsonParser::~SimpleJsonParser(void)
{
}

// ----------------------------------------------------------------------------
//
LPSTR SimpleJsonParser::strip_quotes( LPSTR value ) {

	size_t len = strlen( value );

	if ( value[0] == '"' && value[len - 1] == '"' ) {
        value[len-1] = '\0';
		value++;
	}

    return value;
}

// ----------------------------------------------------------------------------
//
void SimpleJsonParser::parse( FILE* fp )
{
    // These files are not that big- just read into a buffer and treat as a LPCSTR
    CString json;

    fseek( fp, 0L, SEEK_END );
    long size = ftell( fp ) * 2;        // * 2 for newline expansion
    fseek( fp, 0L, SEEK_SET );

    LPSTR buffer = json.GetBufferSetLength( size );

    size = fread( buffer, 1, size, fp );

    json.ReleaseBufferSetLength( size );

    parse( json );
}

// ----------------------------------------------------------------------------
//
void SimpleJsonParser::parse( LPCSTR json_data )
{
	parse( SimpleJsonTokenizer( json_data, "{},[]:", true ) );
}

// ----------------------------------------------------------------------------
//
// JSON sequences supported:
//
// <pair> = tag : <rvalue>
// <object> = { [<pair> [, <pairn>]] }
// <rvalue> = <litteral> | <array> | <object>
// <array> = [ [<object> [,... <objectn>] ] | [ <rvalue> [, ... <rvaluen>] ] | [ <array] [, <arrayn]] ]

void SimpleJsonParser::parse( SimpleJsonTokenizer& tokenizer )
{
#define IS_BREAK( t, b ) (t[0] == b && t[1] == '\0')

    enum ParseState {
        SCAN=1,
        ARRAY,
        PAIR,
        PAIR_COLON,
        RVALUE,
        RVALUE_SEPARATOR
    };

    ParseState state = SCAN;
    char tag_name[MAXTAGNAMELEN+1];
    CString array_index;

	reset();

	push( this );

	while ( tokenizer.hasToken() ) {
		LPSTR token = tokenizer.nextToken();

        switch ( state ) {
            case SCAN:
                if ( IS_BREAK( token, '[' ) ) {                   // Start of array
					setType( JSONARRAY );
                    state = RVALUE;
                    break;
                }
                else if ( IS_BREAK( token, '{' ) ) {              // Start of object
					setType( JSONOBJECT );
                    state = PAIR;
                    break;
                }

                throw std::exception( "Parser expected opening object or array" );

            case PAIR:
                // Check for empty object
                if ( IS_BREAK( token, '}' ) && top()->getType() == JSONOBJECT && top()->valueCount() == 0 ) {
					pop();
                    state = RVALUE_SEPARATOR;
                    break;
                }

                strcpy_s( tag_name, strip_quotes( token ) );
                state = PAIR_COLON;
                break;

            case PAIR_COLON:
                if ( !IS_BREAK( token, ':' ) )
                    throw std::exception( "Parser expecting colon seperator" );
                state = RVALUE;
                break;

            case RVALUE: {
                if ( IS_BREAK( token, ']' ) ) {       // Empty array
                    if ( top()->getType() != JSONARRAY  )
                        throw std::exception( "Unexpected array closing bracket" );

					pop();
                    state = RVALUE_SEPARATOR;
                    break;
                }

                JsonNode* node = top();

                if ( node->getType() == JSONARRAY ) {
					tag_name[0] = '\0';
                }
                else if ( node->has_key( tag_name ) ) {
                    CString error;
                    error.Format( "Duplicate JSON tag name '%s'", tag_name );
                    throw std::exception( (LPCSTR)error );
                }

                if ( IS_BREAK( token, '[' ) ) {
					push( node->setValue( JSONARRAY, tag_name ) );
                    state = RVALUE;
                    break;
                }

                if ( IS_BREAK( token, '{' )) {
					push( node->setValue( JSONOBJECT, tag_name ) );
                    state = PAIR;
                    break;
                }

                // Add a constant to current node container

                if ( node->getType() != JSONOBJECT && node->getType() != JSONARRAY)
                    throw std::exception( "Parser expecting container JSON node" );

                node->setValue( strip_quotes( token ), tag_name );
                state = RVALUE_SEPARATOR;
                break;
            }

            case RVALUE_SEPARATOR: {
                JsonNode* node = m_nodeStack[m_stack_ptr-1];

                if ( IS_BREAK( token, ',' ) ) {
                    state = node->getType() == JSONARRAY ? RVALUE : PAIR;
                    break;
                }

                if ( IS_BREAK( token, '}' ) && node->getType() == JSONOBJECT  ) {
					pop();
                    break;
                }

                if ( IS_BREAK( token, ']' ) && node->getType() == JSONARRAY ) {
					if ( !node->validateArray( ) ) {
						CString error;
						error.Format( "Mixed object types in '%s'", node->getTagName() );
						throw std::exception( (LPCSTR)error );
					}

					pop();
                    break;
                }

                throw std::exception( "Parser expecting object or array seperator" );
            }
        }
    }

    if ( stackSize() > 0 )
        throw std::exception( "Unclosed JSON objects detected" );
}

// ----------------------------------------------------------------------------
//
bool SimpleJsonTokenizer::hasToken() {
	advanceToken();

	return m_token_available || m_break_available;
}

// ----------------------------------------------------------------------------
//
LPSTR SimpleJsonTokenizer::nextToken() {
	if ( m_token_available ) {
		m_token_available = false;
		return m_token;
	}

	if ( m_break_available ) {
		m_break_available = false;
		m_token[0] = m_break_char;
        m_token[1] = '\0';
		return m_token;
	}

	throw std::exception( "No token available" );
}

// ----------------------------------------------------------------------------
//
void SimpleJsonTokenizer::advanceToken() {
	bool in_quotes = false;

	while ( !m_token_available && !m_break_available && *m_head ) {
		if ( in_quotes ) {
			if ( *m_head == '"' && m_last_char != '\\' ) {
				in_quotes = false;
				m_must_break = true;
				extract_token( m_start, m_head );
			}
		}
		else if ( iswspace(*m_head) ) {
			if ( m_in_token && !m_must_break ) {
				extract_token( m_start, m_head-1 );
				m_must_break = true;
			} 
			else if ( !m_in_token )
				m_start = m_head+1;
			else
				; // Ignore WS between tokens and breaks
		}
		else if ( strchr( m_break_chars, *m_head ) != NULL ) {
			if ( m_in_token && !m_must_break )  // Non-quoted literal
				extract_token( m_start, m_head-1 );

			if ( m_in_token )
				m_token_available = true;

			if ( m_store_breaks ) {
				m_break_char = *m_head;
				m_break_available = true;
			}

			m_start = m_head+1;
			m_in_token = m_must_break = false;
		}
		else if ( m_must_break )
			throw std::exception( "Parse error" );
		else {
			m_in_token = true;

			if ( *m_head == '"'  && m_last_char != '\\' )
				in_quotes = true;
		}

		m_last_char = *m_head;

		m_head++;
	}

	if ( m_token_available || m_break_available )
		return;

	if ( m_in_token ) {
		if ( in_quotes )
			throw std::exception( "Unterminate quotes" );

		if ( !m_must_break )  // Non-quoted literal
			extract_token( m_start, m_head-1 );

		m_token_available = true;
		m_in_token = false;
	}
}

// ----------------------------------------------------------------------------
//
void SimpleJsonTokenizer::extract_token( LPCSTR start, LPCSTR end ) {
	unsigned length=end-start+1;

    if ( m_token_buf_len < length+1 ) {
        m_token_buf_len = length+1+256;     // Don't want to double but add some extra
        m_token = (LPSTR)realloc( m_token, m_token_buf_len );
    }

    LPSTR target = m_token;

    while ( start <= end ) {
        *target = *start++;

        if ( *target == '\\' && *start == '\\' )
            start++;

        target++;
    }

    *target = '\0';
}

// ----------------------------------------------------------------------------
//
void JsonNode::dump() {
	for ( JsonNode* node=m_next; node != NULL; node = node->m_next ) {
        printf( "%s(%d) = ", node->getTagName(), node->getType() );

        switch ( node->getType() ) {
            case JSONROOT:
            case JSONOBJECT:
            case JSONARRAY:
                node->dump();
                break;

            case JSONCONSTANT:
				printf( node->getValue() );
                break;

            case JSONNULL:
                printf( "NULL" );
                break;
        }

        printf( "\n----------------------------\n" );
    }
}
