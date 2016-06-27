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

#include "SimpleJsonParser.h"

// ----------------------------------------------------------------------------
//
SimpleJsonParser::SimpleJsonParser(void)
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

    return cstr;
}

// ----------------------------------------------------------------------------
//
CString&  SimpleJsonParser::strip_quotes( CString& value ) {

    if ( value[0] == '"' && value[value.GetLength()-1] == '"' )
        value = value.Mid( 1, value.GetLength()-2 );

    return value;
}

// ----------------------------------------------------------------------------
//
void SimpleJsonParser::parse( LPCSTR json_data )
{
    m_values.clear();

    std::vector<CString> tokens = tokenize( json_data, "{},[]:", true );

    ParseState state = OPENING_BRACE;
    LPCSTR tag_name = NULL;

    // For embedded arrays of objects
    int brackets = 0;
    int braces = 0;

    for ( std::vector<CString>::iterator it=tokens.begin(); it != tokens.end(); ++it ) {
        //printf( "%s\n", (*it) );
        switch ( state ) {
            case OPENING_BRACE:
                if ( (*it) == "[" ) {           // Simple array with no name - eek!
                    tag_name = "";
                    m_values[ tag_name ] = "";
                    state = ARRAY_VALUE_OR_SEPARATOR;
                    brackets=1;
                    break;
                }
                else if ( (*it) != "{" )
                    throw std::exception( "Parser expecting opening brace" );
                state = TAG_NAME;
                break;

            case TAG_NAME:
                tag_name = strip_quotes( (*it) );
                state = COLON;
                break;

            case COLON:
                if ( (*it) != ":" )
                    throw std::exception( "Parser expecting colon seperator" );
                state = TAG_VALUE;
                break;
                
            case TAG_VALUE:
                if ( (*it) == "[" ) {
                    m_values[ tag_name ] = "";                    
                    state = ARRAY_VALUE_OR_SEPARATOR;
                    brackets = 1;
                    break;
                }

                if ( (*it) == "{" ) {
                    m_values[ tag_name ] = "\"";
                    m_values[ tag_name ] += (*it);              
                    state = OBJECT;
                    braces = 1;
                    break;
                }

                m_values[ tag_name ] = strip_quotes( (*it) );
                state = TAG_SEPARATOR;
                break;

            case TAG_SEPARATOR:
                if ( (*it) == "," )
                    state = TAG_NAME;
                else if ( (*it) == "}" )
                    state = DONE;
                else
                    throw std::exception( "Parser expecting tag seperator" );
                break;

            case DONE:
                throw std::exception( "Parser unexpected data after JSON obejct end" );

            case ARRAY_VALUE_OR_SEPARATOR:
                if ( "]" == (*it) ) {
                    brackets--;
                    if ( brackets == 0 )
                        state = TAG_SEPARATOR;
                    break;
                }
                else if ( "{" == (*it) ) {
                    state = ARRAY_OF_OBJECTS;
                    m_values[ tag_name ] = "\"";
                    m_values[ tag_name ] += (*it);
                    braces = 1;
                    break;
                }
                else if ( "[" == (*it) ) {
                    m_values[ tag_name ] += "\"";
                    m_values[ tag_name ] += (*it);
                    brackets++;
                    break;
                }

            case ARRAY_VALUE:
                m_values[ tag_name ] += strip_quotes( (*it) );                    
                state = ARRAY_SEPARATOR;
                break;

            case ARRAY_SEPARATOR:
                if ( "]" == (*it) ) {
                    brackets--;
                    if ( brackets == 0 ) {
                        state = TAG_SEPARATOR;
                        break;
                    }
                    m_values[ tag_name ] += (*it);
                    m_values[ tag_name ] += "\"";
                }
                else if ( (*it) == "," ) {
                    m_values[ tag_name ] += (*it);                    
                    state = ARRAY_VALUE_OR_SEPARATOR;
                }
                else
                    throw std::exception( "Parser expecting array item seperator" );
                break;

            case ARRAY_OF_OBJECTS: {
                if ( "]" == (*it) ) {
                    brackets--;
                    if ( brackets == 0 ) {
                        state = TAG_SEPARATOR;
                        break;
                    }
                }
                else if ( "[" == (*it) )
                    brackets++;
                else if ( "{" == (*it) )
                    braces++;
                else if ( "}" == (*it) )
                    braces--;

                CString escaped ( (*it) );
                escaped.Replace( "\"", "\\\"" );

                m_values[ tag_name ] += escaped;

                if ( !braces )
                    m_values[ tag_name ] += "\"";

                break;
            }

            case OBJECT: {
                if ( "}" == (*it) ) {
                    braces--;
                    if ( braces == 0 ) {
                       m_values[ tag_name ] += (*it);
                       m_values[ tag_name ] += "\"";
                        state = TAG_SEPARATOR;
                        break;
                    }
                }
                else if ( "{" == (*it) )
                    braces++;

                CString escaped ( (*it) );
                escaped.Replace( "\"", "\\\"" );

                m_values[ tag_name ] += escaped;
                break;
            }
        }
    }
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
