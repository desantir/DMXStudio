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

#include "DMXStudio.h"
#include "TextIO.h"

class Form;

class FieldException : public std::exception 
{
    CString	m_message;

public:
    FieldException( const char *format, ... )
    {
        va_list args;
        va_start( args, format );
        m_message.FormatV( format, args );
        va_end( args );
    }

    virtual const char* what() const throw() {
        return (LPCSTR)m_message;
    }
};

class Field
{
    friend class Form;

private:
    bool            m_validated;                // The value is known to be correct
    bool            m_password;

protected:
    CString		    m_label;
    bool			m_hidden;

public:
    Field( LPCSTR label ) :
        m_validated( false ),
        m_label( label ),
        m_hidden(false),
        m_password( false )
    {}

    virtual ~Field() {}
    
    bool isPassword( ) const {
        return m_password;
    }
    void setPassword( bool password ) {
        m_password = password;
    }

    bool isValidated() const {
        return m_validated;
    }
    void setValidated( bool validated ) {
        m_validated = validated;
    }

    bool isHidden( ) const {
        return m_hidden;
    }
    void setHidden( bool hidden ) {
        m_hidden = hidden;
    }

    virtual LPCSTR getLabel(void) {
        return m_label;
    }
    void setLabel( LPCSTR label ) {
        m_label = label;
    }

    virtual void getLabelValue( CString& labelValue ) {
        labelValue = getValue();
    }

    virtual LPCSTR getValue(void) const = 0;
    virtual bool setValue( LPCSTR value ) = 0;
    virtual bool isChanged(void) const = 0;

    virtual void isReady() {                        // Field is ready for form display
    }

    virtual bool isHelp( CString& input ) const {
        return false;
    }
    
    virtual void helpText( CString& help_text) {
    }

    virtual bool nextValue() {
        return false;
    }

    virtual bool previousValue() {
        return false;
    }
};

typedef std::vector< Field* > FormFields;

class Form
{
    TextIO*			m_text_io;

    CString		    m_title;
    FormFields		m_fields;
    bool			m_move_after_change;
    bool			m_auto_delete;					// Form will auto delete fields on clear and destruction
    bool			m_playing;
    bool            m_stop_on_last_field;
    size_t			m_current_field;
    size_t			m_field_width;					// Minimum field width for all fields

    Form( const Form& ) {}
    Form& operator=( const Form& form ) {}

public:
    Form( TextIO* text_io, LPCSTR title=NULL, bool auto_delete=false ) :
        m_text_io( text_io ),
        m_title( title ),
        m_auto_delete( auto_delete ),
        m_move_after_change( true ),
        m_stop_on_last_field( true )
    {}

    virtual ~Form() {
        clear();
    }

    inline bool isStopOnLastField() const {
        return m_stop_on_last_field;
    }
    void setStopOnLastField( bool stop_on_last_field ) {
        m_stop_on_last_field = stop_on_last_field;
    }

    void setTitle( LPCSTR title ) {
        m_title = title;
    }

    size_t size() const {
        return m_fields.size();
    }

    void add( Field& field ) {
        STUDIO_ASSERT( !m_auto_delete, "Non-delete field added, but auto deleting" );

        m_fields.push_back( &field );
    }

    void setFieldCount( size_t num_fields ) {
        STUDIO_ASSERT( num_fields <= size(), "Attempting to set field count < current size" );

        m_fields.resize( num_fields );
    }

    void addAuto( Field* fieldp ) {
        STUDIO_ASSERT( m_auto_delete, "Auto delete field added, but not deleting" );
        m_fields.push_back( fieldp );
    }

    template<class T>
    T* getField( size_t field_num ) {
        STUDIO_ASSERT( field_num < m_fields.size(), "Field out of range" );
        T* f = dynamic_cast<T*>( m_fields[field_num] );
        return f;
    }

    virtual bool play( void );

    void stop() {
        m_playing = false;
    }

    virtual void clear() {
        if ( m_auto_delete ) {
            for ( FormFields::iterator it=m_fields.begin(); it != m_fields.end(); ++it ) {
                delete (*it);
            }
        }

        m_fields.clear();
    }

    bool isChanged() const {
        for ( UINT field_num=0; field_num < m_fields.size(); field_num++ ) {
            if ( m_fields[field_num]->isChanged() )
                return true;
        }
        return false;
    }

    virtual void fieldChangeNotify( size_t field_num ) { }
    virtual void fieldLeaveNotify( size_t field_num ) { }
    virtual void formCompleteNotify() {}

    bool isMoveAfterChange() const {
        return m_move_after_change;
    }
    void setMoveAfterChange( bool move_after_change ) {
        m_move_after_change = move_after_change;
    }

    bool isAutoDelete( ) const {
        return m_auto_delete;
    }
    void setAutoDelete( bool auto_delete ) {
        STUDIO_ASSERT( m_fields.size() == 0, "Cannot change auto delete while fields exist" );
        m_auto_delete = auto_delete;
    }

    virtual void formatFieldLabel( CString& label, Field *field ) {
        CString title( field->getLabel() );
        size_t width = title.GetLength();

        if ( width < m_field_width ) {
            title += " ";
            while ( ++width < m_field_width )
                title += "_";
        }

        CString labelValue;
        field->getLabelValue( labelValue );
        label.Format( "%s [%s]", title, 
            (labelValue.GetLength() > 0 && field->isPassword()) ? "******" : labelValue );
    }

private:
    size_t first_field() const;
    size_t last_field() const;
    size_t next_field( size_t current_field ) const;
    size_t previous_field( size_t current_field ) const;

     void validateField( size_t field_num ) {
        STUDIO_ASSERT( field_num < m_fields.size(), "Invalidating invalid field number" );
        Field* field = m_fields[field_num];
        field->setValue( field->getValue() );       // Will throw exception if value is invalid
    }
};

class InputField : public Field
{
protected:
    CString				m_default_value;
    CString				m_current_value;

public:
    InputField( LPCSTR label, LPCSTR default_value ) :
        Field( label ),
        m_default_value( default_value ),
        m_current_value( default_value )
    {}

    LPCSTR getValue() const {
        return (LPCSTR)m_current_value;
    }

    bool setValue( LPCSTR value ) {
        m_current_value = value;
        return true;
    }

    void setDefaultValue( LPCSTR value ) {
        m_default_value = value;
    }

    bool isChanged() const {
        return m_default_value != m_current_value;
    }
};

typedef std::vector<CString> SelectionList;

class SelectionField : public InputField
{
protected:
    SelectionList	m_selections;
    bool			m_must_match;

    SelectionField( LPCSTR label, LPCSTR default_value, bool must_match = false ) :
        InputField( label, default_value ),
        m_must_match( must_match )
    {}

public:
    SelectionField( LPCSTR label, LPCSTR default_value, SelectionList list, bool must_match = false ) :
        InputField( label, default_value ),
        m_selections( list ),
        m_must_match( must_match )
    {}

    int getValueIndex( ) {
        int index = 0;

        for ( SelectionList::iterator it=m_selections.begin(); it != m_selections.end(); it++, index++ )
            if ( _stricmp( (*it), m_current_value ) == 0 )
                return index;

        return -1;
    }

    bool setValue( LPCSTR value ) {
        if ( m_must_match ) {
            bool match = false;

            for ( SelectionList::iterator it=m_selections.begin(); it != m_selections.end(); ++it ) {
                if ( _stricmp( (*it), m_current_value ) == 0 ) {
                    match = true;
                    break;
                }
            }

            if ( !match ) 
                throw FieldException( "Invalid value -- type '?' for valid selections" );
        }

        return InputField::setValue( value );
    }

    bool nextValue() {
        if ( m_selections.size() == 0 )
            return false;
        int index = getValueIndex();
        if ( index == m_selections.size()-1 )
            return false;
        if ( index == -1 )
            index = 0;
        else
            index++;
        return setValue( m_selections[index] );
    }

    bool previousValue() {
        if ( m_selections.size() == 0 )
            return false;
        int index = getValueIndex();
        if ( index == 0 )
            return false;
        if ( index < 1 )
            index = m_selections.size()-1;
        else
            index--;
        return setValue( m_selections[index] );
    }

    bool isHelp( CString& input ) const {
        return _stricmp( input, "?" ) == 0;
    }

    void helpText( CString& help_text) {
        help_text = "";
        for ( SelectionList::iterator it=m_selections.begin(); it != m_selections.end(); ++it )
            help_text.AppendFormat( "%s\n", (LPCSTR)(*it) );
    }
};

class IntegerField : public InputField
{
    long	m_value;			// Shadows text value
    long	m_low;
    long	m_high;

public:
    IntegerField( int low, int high ) :
        InputField( "","" ),
        m_low( low ),
        m_high( high )
    {
    }

    IntegerField( LPCSTR label, long default_value, 
                  long low=LONG_MIN, long high=LONG_MAX ) :
        InputField( label, "" ),
        m_low( low ),
        m_high( high )
    {
        setInitialValue( default_value );
    }

    int getIntValue() const {
        return (int)m_value;
    }

    long getLongValue() const {
        return m_value;
    }

    bool nextValue() {
        if ( m_value == m_high )
            return false;
        return setValue( m_value+1 );
    }

    bool previousValue() {
        if ( m_value == m_low )
            return false;
        return setValue( m_value-1 );
    }

    bool setValue( LPCSTR value ) {
        if ( !parse( value, m_value ) )
            return false;
        return InputField::setValue( value );
    }

    bool setValue( long value ) {
        CString value_str;
        value_str.Format( "%ld", value );
        return setValue( value_str );
    }

    void setHigh( long high ) {
        m_high = high;
    }

    void setLow( long low ) {
        m_low = low;
    }

    void setInitialValue( long value ) {
        setValue( value );
        m_default_value = m_current_value;
    }

protected:
    bool parse( LPCSTR value, long& result ) {
        for ( LPCSTR c=value; *c; c++ )
            if ( !isdigit( *c ) )
                return false;
        long l = atol( value );
        if ( l < m_low || l > m_high )
            return false;
        result = l;
        return true;
    }
};

class FloatField : public InputField
{
    float   m_value;			// Shadows text value
    LPCSTR  m_format;           // Output format

public:
    FloatField( LPCSTR label, float default_value, LPCSTR format="%f" ) :
        InputField( label, "" ),
        m_format( format )
    {
        setInitialValue( default_value );
    }

    float getFloatValue() const {
        return m_value;
    }

    bool setValue( LPCSTR value ) {
        if ( !parse( value, m_value ) )
            return false;

        CString value_str;
        value_str.Format( m_format, m_value );
        return InputField::setValue( value_str );
    }

    bool setValue( float value ) {
        CString value_str;
        value_str.Format( m_format, value );


        return setValue( value_str );
    }

    void setInitialValue( float value ) {
        setValue( value );
        m_default_value = m_current_value;
    }

protected:
    bool parse( LPCSTR value, float& result ) {
        float d;
        if ( 1 != sscanf_s( value, "%f", &d ) )
            return false;
        result = d;
        return true;
    }
};

class BooleanField : public InputField
{
public:
    BooleanField() : InputField( "","" )
    {
    }

    BooleanField( LPCSTR label, bool default_value ) : InputField( label, "" )
    {
        setInitialValue( default_value );
    }

    bool isSet() const {
        return _stricmp( "yes", getValue() ) == 0;
    }

    bool nextValue() {
        return setValue( !isSet() );
    }

    bool previousValue() {
        return setValue( !isSet() );
    }

    bool setValue( LPCSTR value ) {
        if ( !_stricmp( value, "yes" ) || !_stricmp( value, "true" ) ||
             !_stricmp( value, "y" ) || !_stricmp( value, "t" ) || !_stricmp( value, "1" ) ) {
            value = "yes";
        }
        else if ( !_stricmp( value, "no" ) || !_stricmp( value, "false" ) ||
                  !_stricmp( value, "n" ) || !_stricmp( value, "f" ) || !_stricmp( value, "0" ) ) {
            value = "no";
        }
        else {
            throw FieldException( "Invalid value -- choose yes or no" );
        }

        return InputField::setValue( value );
    }

    void setInitialValue( bool value ) {
        setValue( value );
        m_default_value = m_current_value;
    }

    bool setValue( bool value ) {
        return setValue( value ? "yes" : "no" );
    }
};

class KeyListComparator
{
public:
    bool operator() ( const CString& k1, const CString& k2 ) {
        if ( k1.GetLength() < k2.GetLength() )
            return true;
        if ( k1.GetLength() > k2.GetLength() )
            return false;
        return k1 < k2;
    }
};

class KeyListField : public InputField 
{
protected:
    typedef std::map<CString,CString,KeyListComparator> KeyListMap;
    
    KeyListMap		m_selections;

    KeyListField( LPCSTR label ) :
        InputField( label, "" )
    {}

public:
    void addKeyValue( LPCSTR key, LPCSTR value ) {
        m_selections[ key ] = value;
    }

    LPCSTR getKeyValue( ) {
        return m_selections[ getValue() ];
    }

    void clear() {
        m_selections.clear();
    }

    bool nextValue() {
        KeyListMap::iterator it = m_selections.find( m_current_value );
        if ( it == m_selections.end() )
            return false;
        it++;
        if ( it == m_selections.end() )
            return false;
        return setValue( (*it).first );
    }

    bool previousValue() {
        KeyListMap::iterator it = m_selections.find( m_current_value );
        if ( it == m_selections.begin() )
            return false;
        it--;
        return setValue( (*it).first );
    }

    void setDefaultListValue( CString selection ) {
        m_default_value = m_current_value = selection;
    }

    void getLabelValue( CString& labelValue ) {
        KeyListMap::iterator it = m_selections.find( m_current_value );
        if ( it != m_selections.end() ) {
            labelValue.Format( "%s-%s", (*it).first, (LPCSTR)(*it).second );
            return;
        }
        labelValue = "INTERNAL ERROR";
    }

    bool setValue( LPCSTR value ) {
        if ( m_selections.size() == 0 )
            return false;

        CString key( value );
        key.MakeUpper();

        KeyListMap::iterator it = m_selections.find( key );
        if ( it == m_selections.end() )
            throw FieldException( "Invalid selection '%s'- type '?' for value list", (LPCSTR)value );

        InputField::setValue( key );
        return true;
    }

    bool isHelp( CString& input ) const {
        return _stricmp( input, "?" ) == 0;
    }

    void helpText( CString& help_text) {
        UINT maxlen = 0;
        for ( KeyListMap::iterator it=m_selections.begin(); it != m_selections.end(); ++it )
            maxlen = std::max<UINT>( (*it).first.GetLength(), maxlen );

        CString format;
        format.Format( "   %%%ds - %%s\n", maxlen );

        help_text = "";
        for ( KeyListMap::iterator it=m_selections.begin(); it != m_selections.end(); ++it )
            help_text.AppendFormat( (LPCSTR)format, (*it).first, (*it).second );
    }
};

class NumberedListField : public KeyListField
{
public:
    NumberedListField( LPCSTR label ) :
        KeyListField( label )
    {}

    DWORD getListValue() const {
        return strtoul( (LPCSTR)getValue(), NULL, 10 );
    }

    void setDefaultListValue( DWORD selection ) {
        CString current_selection;
        current_selection.Format( "%lu", selection );

        try {
            setValue( current_selection );
        }
        catch ( ... ) {
            setValue( m_selections.begin()->first );
        }
    }

    void addKeyValue( DWORD key, LPCSTR value ) {
        CString key_value;
        key_value.Format( "%lu", key );
        KeyListField::addKeyValue( key_value, value );
    }
};

class MultiKeyListField : public KeyListField 
{
protected:
    std::vector<CString>   m_selected;

    MultiKeyListField( LPCSTR label ) :
        KeyListField( label )
    {
    }

public:
    MultiKeyListField( LPCSTR label, std::vector<CString> selected ) :
        KeyListField( label ),
        m_selected( selected )
    {
        setDefaultListValue( selected );
    }

    void addKeyValue( LPCSTR key, LPCSTR value ) {
        m_selections[ key ] = value;
    }

    std::vector<CString> getSelections( ) const {
        return m_selected;
    }

    bool nextValue() {
        return false;
    }

    bool previousValue() {
        return false;
    }

    void setDefaultListValue( std::vector<CString> selected ) {
        CString value;

        for ( std::vector<CString>::iterator it=selected.begin(); it != selected.end(); ++it ) {
            if ( value.GetLength() > 0 )
                value.Append( "," );
            value.AppendFormat( "%s", (LPCSTR)(*it) );
        }

        m_selected = selected;
        m_default_value = m_current_value = value;
    }    

    void getLabelValue( CString& labelValue ) {
        labelValue = getValue();
    }

    bool setValue( LPCSTR value ) {
        if ( m_selections.size() == 0 )
            return false;

        CString key( value );
        std::vector<CString> selected;

        int curPos = 0;
        while ( true ) {
            CString resToken = key.Tokenize( _T(" ,"), curPos );
            if ( resToken.IsEmpty() )
                break;

            KeyListMap::iterator it = m_selections.find( resToken );
            if ( it == m_selections.end() )
                throw FieldException( "Invalid selection '%s'- type '?' for value list", (LPCSTR)value );

            selected.push_back( resToken );
        }

        m_selected = selected;

        if ( selected.size() == 0 )
            key.Empty();

        return InputField::setValue( key );
    }
};

class MultiNumberedListField : public MultiKeyListField 
{

public:
    MultiNumberedListField( LPCSTR label ) :
        MultiKeyListField( label )
    {
    }

    void setDefaultListValue( std::vector<UINT> selected ) {
        std::vector<CString> selections;

        for ( std::vector<UINT>::iterator it=selected.begin(); it != selected.end(); ++it ) {
            CString value;
            value.Format( "%d", (*it) );
            selections.push_back( value );
        }

        MultiKeyListField::setDefaultListValue( selections );
    }  

    void addKeyValue( UINT key, LPCSTR value ) {
        CString key_string;
        key_string.Format( "%d", key );
        MultiKeyListField::addKeyValue( key_string, value );
    }

    std::vector<UINT> getIntSelections( ) const {
        std::vector<UINT> int_selections;

        for ( CString item : getSelections() )
            int_selections.push_back( (UINT)atol( item ) );

        return int_selections;
    }
};
