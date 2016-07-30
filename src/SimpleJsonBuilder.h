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
#include "RGBWA.h"

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

class JsonWriter
{
    public:
        virtual void Append( LPCSTR value ) = 0;
        virtual void AppendFormat( LPCSTR format, ... ) = 0;
};

class JsonStringWriter : public JsonWriter
{
    CString*        m_buffer;

public:
    JsonStringWriter() : 
        m_buffer( NULL )
    {}

    JsonStringWriter( CString& buffer ) : 
        m_buffer( &buffer )
    {}

    inline void Append( LPCSTR value ) {
        m_buffer->Append( value );
    }

    void AppendFormat( LPCSTR format, ... ) {
        va_list _ArgList;
        __crt_va_start(_ArgList, format);
        m_buffer->AppendFormatV( format, _ArgList );
        __crt_va_end(_ArgList);    
    }
};

class JsonFileWriter : public JsonWriter
{
    FILE*       m_fp;

public:
    JsonFileWriter( LPCSTR file_name ) {
        errno_t err = fopen_s( &m_fp, file_name, "w" );

        if ( err || !m_fp ) {
            CString error;
            error.Format( "Unable to open file %s for write", file_name );
            throw std::exception( error );
        }
    }

    ~JsonFileWriter() {
        fflush( m_fp );
        fclose( m_fp );
    }

    inline void Append( LPCSTR value ) {
        fwrite( value, 1, strlen( value ), m_fp );
    }

    void AppendFormat( LPCSTR format, ... ) {
        va_list _ArgList;
        __crt_va_start(_ArgList, format);
        vfprintf_s( m_fp, format, _ArgList );
        __crt_va_end(_ArgList);    
    }
};

class JsonBuilder
{
    JsonStringWriter    m_stringWriter;
    JsonWriter&         m_buffer;
    bool                m_pretty;

    std::vector<JsonObject> m_stack;

public:
    JsonBuilder( CString& buffer, bool pretty = false ) : 
        m_stringWriter( buffer ),
        m_pretty( pretty ),
        m_buffer( m_stringWriter )
    {}

    JsonBuilder( JsonWriter& writer, bool pretty = false ) : 
        m_pretty( pretty ),
        m_buffer( writer )
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
        m_buffer.AppendFormat( "\"%s\": {", name );
        m_stack.push_back( JsonObject( false, name ) );
    }

    void startArray( ) {
        addSeparator();
        m_buffer.Append( "[" );
        m_stack.push_back( JsonObject( true, "" ) );
        indent();
    }

    void startArray( LPCSTR name ) {
        addSeparator();
        m_buffer.AppendFormat( "\"%s\": [", name );
        m_stack.push_back( JsonObject( true, name ) );
    }

    void endArray( LPCSTR name ) {
        if ( m_stack.size() == 0 )
            throw std::exception( "END ARRAY ON EMPTY STACK" );
        else {
            JsonObject jobj = m_stack.back();
            if ( !jobj.isArray() ) 
                throw std::exception( "EXPECTING ARRAY" );
            if ( name != NULL && !jobj.isName( name ) ) {
                CString error;
                error.Format( "EXPECTING ARRAY NAME %s", name );
                throw std::exception( error );
            }

            m_stack.pop_back();
            indent();
            m_buffer.Append( "]" );
        }
    }

    inline void endArray(  ) {
       endArray( NULL );
    }

    void endObject( LPCSTR name ) {
        if ( m_stack.size() == 0 ) 
            throw std::exception( "END OBJECT ON EMPTY STACK" );
        else {
            JsonObject jobj = m_stack.back();
            if ( jobj.isArray() ) 
                throw std::exception( "EXPECTING OBJECT" );
            if ( name != NULL && !jobj.isName( name ) ) {
                CString error;
                error.Format( "EXPECTING OBJECT NAME %s", name );
                throw std::exception( error );
            }

            m_stack.pop_back();
            indent();
            m_buffer.Append( "}" );
       }
    }

    inline void endObject() {
        endObject( NULL );
    } 

    void addNull( LPCSTR name) {
        addSeparator();
        m_buffer.AppendFormat( "\"%s\": null", name );
    }

    void add( LPCSTR name, int value ) {
        addSeparator();
        m_buffer.AppendFormat( "\"%s\": %d", name, value );
    }

    void add( LPCSTR name, unsigned value ) {
        addSeparator();
        m_buffer.AppendFormat( "\"%s\": %u", name, value );
    }

    void add( LPCSTR name, double value ) {
        addSeparator();
        m_buffer.AppendFormat( "\"%s\": %lf", name, value );
    }
    
    void add( LPCSTR name, LPCSTR value ) {
        addSeparator();
        m_buffer.AppendFormat( "\"%s\": \"%s\"", name, JsonObject::encodeJsonString(value) );
    } 

    void add( LPCSTR name, RGBWA value ) {
        addSeparator();
        m_buffer.AppendFormat( "\"%s\": \"%06lX\"", name, (ULONG)value );
    } 

    void add( LPCSTR name, ULONG value ) {
        addSeparator();
        m_buffer.AppendFormat( "\"%s\": %lu", name, value );
    } 

    void add( LPCSTR name, bool value ) {
        addSeparator();
        m_buffer.AppendFormat( "\"%s\": %s", name, (value) ? "true" : "false" );
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
        if ( m_stack.size() > 0 && m_stack.back().needSeparator() ) {
            m_buffer.Append( "," );
        }

        indent();
    }

    inline void indent() {
        if ( m_pretty ) {
            m_buffer.Append( "\n" );

            for ( int i=m_stack.size(); i--; )
                m_buffer.Append( "    " );
        }
    }
};
