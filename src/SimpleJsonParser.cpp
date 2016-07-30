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

#include "stdafx.h"
#include "SimpleJsonParser.h"

// ----------------------------------------------------------------------------
//
SimpleJsonParser::SimpleJsonParser(void) :
    JsonNode( JsonNodeType::JSONROOT )
{
}

// ----------------------------------------------------------------------------
//
SimpleJsonParser::~SimpleJsonParser(void)
{
}

// ----------------------------------------------------------------------------
//
CString extract_token( LPCSTR start, LPCSTR end ) {
    CString cstr;
    unsigned length=end-start+1;

    LPSTR buffer = cstr.GetBufferSetLength( length+1 );
    memcpy( buffer, start, length );
    buffer[length] = '\0';
    cstr.ReleaseBuffer();
    cstr.Replace( "\\\\", "\\" );
    return cstr;
}

// ----------------------------------------------------------------------------
//
CString& SimpleJsonParser::strip_quotes( CString& value ) {

    if ( value[0] == '"' && value[value.GetLength()-1] == '"' )
        value = value.Mid( 1, value.GetLength()-2 );

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
   parse( tokenize( json_data, "{},[]:", true ) );
}

// ----------------------------------------------------------------------------
//
// JSON sequences supported:
//
// <pair> = tag : <rvalue>
// <object> = { [<pair> [, <pairn>]] }
// <rvalue> = <litteral> | <array> | <object>
// <array> = [ [<object> [,... <objectn>] ] | [ <rvalue> [, ... <rvaluen>] ] | [ <array] [, <arrayn]] ]

void SimpleJsonParser::parse( std::vector<CString>& tokens )
{
    enum ParseState {
        SCAN=1,
        ARRAY,
        PAIR,
        PAIR_COLON,
        RVALUE,
        RVALUE_SEPARATOR
    };

    ParseState state = SCAN;
    LPCSTR tag_name = NULL;
    CString array_index;
    std::stack<JsonNode *> nodeStack;

    m_values.clear();
    m_type = JSONROOT;

    nodeStack.push( this );

    for ( CString& token : tokens ) {
        // printf( "%s ", (LPCSTR)token );

        switch ( state ) {
            case SCAN:
                if ( token == "[" ) {                   // Start of array
                    m_type = JSONARRAY;
                    state = RVALUE;
                    break;
                }
                else if ( token == "{" ) {              // Start of object
                    m_type = JSONOBJECT;
                    state = PAIR;
                    break;
                }

                throw std::exception( "Parser expected opening object or array" );

            case PAIR:
                // Check for empty object
                if ( token == "}" && nodeStack.top()->m_type == JSONOBJECT && nodeStack.top()->m_values.size() == 0 ) {
                    nodeStack.pop();
                    state = RVALUE_SEPARATOR;
                    break;
                }

                tag_name = strip_quotes( token );
                state = PAIR_COLON;
                break;

            case PAIR_COLON:
                if ( token != ":" )
                    throw std::exception( "Parser expecting colon seperator" );
                state = RVALUE;
                break;

            case RVALUE: {
                if ( token == "]" ) {       // Empty array
                    if ( nodeStack.top()->m_type != JSONARRAY  )
                        throw std::exception( "Unexpected array closing bracket" );

                    nodeStack.pop();
                    state = RVALUE_SEPARATOR;
                    break;
                }

                JsonNode* node = nodeStack.top();

                if ( node->m_type == JSONARRAY ) {
                    array_index.Format( "%d", node->m_values.size() );
                    tag_name = (LPCSTR)array_index;
                }

                if ( node->m_values.find( tag_name ) != node->m_values.end() ) {
                    CString error;
                    error.Format( "Duplicate JSON tag name '%s'", tag_name );
                    throw std::exception( (LPCSTR)error );
                }

                if ( token == "[" ) {
                    node->m_values[ tag_name ] = JsonNode( JSONARRAY );
                    state = RVALUE;
                    nodeStack.push( &node->m_values[ tag_name ] );
                    break;
                }

                if ( token == "{" ) {
                    node->m_values[ tag_name ] = JsonNode( JSONOBJECT );
                    state = PAIR;
                    nodeStack.push( &node->m_values[ tag_name ] );
                    break;
                }

                // Add a constant to current node container

                if ( node->m_type != JSONOBJECT && node->m_type != JSONARRAY)
                    throw std::exception( "Parser expecting container JSON node" );

                node->m_values[ tag_name ] = JsonNode( strip_quotes( token ) );
                state = RVALUE_SEPARATOR;
                break;
            }

            case RVALUE_SEPARATOR: {
                JsonNode* node = nodeStack.top();

                if ( token == "," ) {
                    state = node->m_type == JSONARRAY ? RVALUE : PAIR;
                    break;
                }

                if ( token == "}" && node->m_type == JSONOBJECT  ) {
                    nodeStack.pop();
                    break;
                }

                if ( token == "]" && node->m_type == JSONARRAY ) {
                    // Assert all non-null nodes in the array are the same type
                    JsonNodeType expected_type = JSONNULL;
                    for ( auto child : node->m_values ) {
                        if ( child.second.m_type == JSONNULL )
                            ;
                        else if ( expected_type == JSONNULL )
                            expected_type = child.second.m_type;
                        else if ( child.second.m_type != expected_type ) {
                            CString error;
                            error.Format( "Mixed object types in '%s'", child.first );
                            throw std::exception( (LPCSTR)error );
                        }
                    }

                    nodeStack.pop();
                    break;
                }

                throw std::exception( "Parser expecting object or array seperator" );
            }
        }
    }

    if ( nodeStack.size() > 0 )
        throw std::exception( "Unclosed JSON objects detected" );
}

// ----------------------------------------------------------------------------
//
std::vector<CString> SimpleJsonParser::tokenize( LPCSTR value, LPCSTR break_chars, bool store_breaks ) 
{
    std::vector<CString> tokens;

    LPCSTR start;
    LPCSTR head;
    CString token;

    bool in_quotes = false;
    bool in_token = false;
    bool must_break = false;

    char last_char = '\0';

    for ( head=start=value; *head; head++ ) {
        if ( in_quotes ) {
            if ( *head == '"' && last_char != '\\' ) {
                in_quotes = false;
                must_break = true;
                token = extract_token( start, head );
            }
        }
        else if ( iswspace(*head) ) {
            if ( in_token && !must_break ) {
                token = extract_token( start, head-1 );
                must_break = true;
            } 
            else if ( !in_token )
                start = head+1;
            else
                ; // Ignore WS between tokens and breaks
        }
        else if ( strchr( break_chars, *head ) != NULL ) {
            if ( in_token && !must_break )  // Non-quoted literal
                token = extract_token( start, head-1 );

            if ( in_token )
                tokens.push_back( token );

            if ( store_breaks )
                tokens.push_back( extract_token( head, head ) );

            start = head+1;
            in_token = must_break = false;
        }
        else if ( must_break )
            throw std::exception( "Parse error" );
        else {
            in_token = true;
            
            if ( *head == '"'  && last_char != '\\' )
                in_quotes = true;
        }

        last_char = *head;
    }
    
    if ( in_token ) {
        if ( in_quotes )
            throw std::exception( "Unterminate quotes" );

        if ( !must_break )  // Non-quoted literal
            token = extract_token( start, head-1 );

        tokens.push_back( token );
    }

    return tokens;
}

// ----------------------------------------------------------------------------
//
void JsonNode::dump() {
    for ( NAME_VALUE_PAIR::iterator it = m_values.begin(); it != m_values.end(); ++it ) {
        printf( "%s(%d) = ", (LPCSTR)it->first, it->second.m_type );

        it->second.dump();

        printf( "\n----------------------------\n" );
    }
}
