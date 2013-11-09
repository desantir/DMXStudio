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

class JsonObject
{
    bool        m_isArray;
    CString     m_name;
    bool        m_first;

public:
    JsonObject( bool array, LPCSTR name ) :
        m_isArray( array ),
        m_name( name ),
        m_first( true )
    {}

    inline bool needSeparator() {
        bool need = !m_first;
        m_first = false;
        return need;
    }

    inline bool isArray() const {
        return m_isArray;
    }

    inline bool isName( LPCSTR name ) const {
        return m_name.Compare( name ) == 0;
    }

    static CString encodeJsonString( LPCSTR string ) {
        CString result( string );
        result.Replace( "\\", "\\\\" );
        result.Replace( "\"", "\\\"" );
        result.Replace( "\n", "\\n" );
        result.Replace( "\r", "\\r" );
        return result;
    }
};

class JsonBuilder
{
    CString&       m_buffer;

    std::vector<JsonObject> m_stack;


public:
    JsonBuilder( CString& buffer ) : 
        m_buffer( buffer )
    {}

    ~JsonBuilder() {
        if ( m_stack.size() > 0 )
            printf( "%d UNRESOLVED JSON OBJECTS", m_stack.size() );
    }
       
    void startObject( ) {
        addSeparator();
        m_buffer.Append( "{" );
        m_stack.push_back( JsonObject( false, "" ) );
    }

    void startObject( LPCSTR name ) {
        addSeparator();
        m_buffer.AppendFormat( "\"%s\":{", name );
        m_stack.push_back( JsonObject( false, name ) );
    }

    void startArray( ) {
        addSeparator();
        m_buffer.Append( "[" );
        m_stack.push_back( JsonObject( true, "" ) );
    }

    void startArray( LPCSTR name ) {
        addSeparator();
        m_buffer.AppendFormat( "\"%s\":[", name );
        m_stack.push_back( JsonObject( true, name ) );
    }

    void endArray( LPCSTR name ) {
        JsonObject jobj = m_stack.back();
        if ( !jobj.isArray() ) 
            printf( "EXPECTING ARRAY\n" );
        if ( name != NULL && !jobj.isName( name ) ) 
            printf( "EXPECTING ARRAY NAME %s\n", name );
        m_buffer.Append( "]" );
        m_stack.pop_back();
    }

    inline void endArray(  ) {
       endArray( NULL );
    }

    void endObject( LPCSTR name ) {
        JsonObject jobj = m_stack.back();
        if ( jobj.isArray() ) 
            printf( "EXPECTING OBJECT\n" );
        if ( name != NULL && !jobj.isName( name ) ) 
            printf( "EXPECTING OBJECT NAME %s\n", name );
        m_buffer.Append( "}" );
        m_stack.pop_back();
    }

    inline void endObject() {
        endObject( NULL );
    } 

    void addNull( LPCSTR name) {
        addSeparator();
        m_buffer.AppendFormat( "\"%s\":null", name );
    }

    void add( LPCSTR name, int value ) {
        addSeparator();
        m_buffer.AppendFormat( "\"%s\":%d", name, value );
    }

    void add( LPCSTR name, unsigned value ) {
        addSeparator();
        m_buffer.AppendFormat( "\"%s\":%u", name, value );
    }

    void add( LPCSTR name, double value ) {
        addSeparator();
        m_buffer.AppendFormat( "\"%s\":%lf", name, value );
    }
    
    void add( LPCSTR name, LPCSTR value ) {
        addSeparator();
        m_buffer.AppendFormat( "\"%s\":\"%s\"", name, JsonObject::encodeJsonString(value) );
    } 

    void addColor( LPCSTR name, unsigned value ) {
        addSeparator();
        m_buffer.AppendFormat( "\"%s\":\"%06lX\"", name, value );
    } 

    void add( LPCSTR name, ULONG value ) {
        addSeparator();
        m_buffer.AppendFormat( "\"%s\":%lu", name, value );
    } 

    void add( LPCSTR name, bool value ) {
        addSeparator();
        m_buffer.AppendFormat( "\"%s\":%s", name, (value) ? "true" : "false" );
    }
    
    void add( int value ) {
        addSeparator();
        m_buffer.AppendFormat( "%d", value );
    }
    
    void add( LPCSTR value ) {
        addSeparator();
        m_buffer.AppendFormat( "\"%s\"", JsonObject::encodeJsonString(value) );
    }   
    
    void add( ULONG value ) {
        addSeparator();
        m_buffer.AppendFormat( "%lu", value );
    }
    
    void add( bool value ) {
        addSeparator();
        m_buffer.AppendFormat( "%s", (value) ? "true" : "false" );
    }

    void add( unsigned value ) {
        addSeparator();
        m_buffer.AppendFormat( "%u", value );
    }

    void add( double value ) {
        addSeparator();
        m_buffer.AppendFormat( "%lf", value );
    }

    void addNull() {
        addSeparator();
        m_buffer.Append( "null" );
    }
    
    void addColor( unsigned value ) {
        addSeparator();
        m_buffer.AppendFormat( "\"%06lX\"", value );
    } 
                                 
    template <class T>
    void addArray( LPSTR name, T values ) {
        startArray( name );
        for ( T::iterator it=values.begin(); it != values.end(); it++ )
            add( (*it) );
        endArray();
    }

    template <class T>
    void addColorArray( LPSTR name, T values ) {
        startArray( name );
        for ( T::iterator it=values.begin(); it != values.end(); it++ )
            addColor( (*it) );
        endArray();
    }

private:
    inline void addSeparator() {
        if ( m_stack.size() > 0 && m_stack.back().needSeparator() )
            m_buffer.Append( "," );
    }
};
