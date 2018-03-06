/* 
    Copyright (C) 2011-2016 Robert DeSantis
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

#include "SceneChannelAnimator.h"

const char* SceneChannelAnimator::className = "SceneChannelAnimator";
const char* SceneChannelAnimator::animationName = "Channel program";

// ----------------------------------------------------------------------------
//
SceneChannelAnimator::SceneChannelAnimator( UID animation_uid, bool shared, UID reference_fixture, 
                                            AnimationSignal signal,
                                            ChannelAnimationArray& channel_animations ) :
    AnimationDefinition( animation_uid, shared, reference_fixture, signal ),
    m_channel_animations( channel_animations )
{
}

// ----------------------------------------------------------------------------
//
SceneChannelAnimator::SceneChannelAnimator( UID animation_uid, bool shared, UID reference_fixture, 
                                            AnimationSignal signal ) :
    AnimationDefinition( animation_uid, shared, reference_fixture, signal )
{
}

// ----------------------------------------------------------------------------
//
AnimationDefinition* SceneChannelAnimator::clone( ) {
	return new SceneChannelAnimator( 0L, m_shared, m_reference_fixture, m_signal );
}

// ----------------------------------------------------------------------------
//
SceneChannelAnimator::~SceneChannelAnimator(void)
{
}

// ----------------------------------------------------------------------------
//
CString SceneChannelAnimator::getSynopsis(void) {
    CString synopsis;

    synopsis.Format( "Channels( " );

    bool first = true;
    for ( ChannelAnimation& chan_anim : m_channel_animations ) {
        if ( !first )
            synopsis.Append( "\n         " );
        else
            first = false;

        synopsis.AppendFormat( "%s", chan_anim.getSynopsis() );
    }

    synopsis += " )";

    return synopsis;
}

// ----------------------------------------------------------------------------
//
ChannelAnimation::ChannelAnimation( channel_address channel, 
    ChannelAnimationStyle animation_style,
    ChannelValueArray& value_list ) :
    m_channel( channel ),
    m_animation_style( animation_style ),
    m_value_list( value_list )
{
}

// ----------------------------------------------------------------------------
//
ChannelAnimation::ChannelAnimation( channel_address channel, 
    ChannelAnimationStyle animation_style ) :
    m_channel( channel ),
    m_animation_style( animation_style )
{
    m_value_list.reserve( 500 );
}

// ----------------------------------------------------------------------------
//
CString ChannelAnimation::getSynopsis(void) {
    CString synopsis;
    CString style;

    switch ( m_animation_style ) {
    case CAM_LIST:	style = "value list="; break;
    case CAM_RANGE:	style = "value range="; break;
    case CAM_SCALE:	style = "scale value"; break;
    case CAM_LEVEL:	style = "level value"; break;
    }

    synopsis.Format( "channel=%d %s", m_channel+1, style );

    if ( m_value_list.size() > 0 ) {
        for ( ChannelValueArray::iterator it=m_value_list.begin(); 
        it != m_value_list.end(); ++it ) {
            if ( it != m_value_list.begin() )
                synopsis.Append( "," );
            synopsis.AppendFormat( "%u", (unsigned)(*it) );
        }
        synopsis.Append( " " );
    }

    return synopsis;
}