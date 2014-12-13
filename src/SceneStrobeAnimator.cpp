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


#include "SceneStrobeAnimator.h"

const char* SceneStrobeAnimator::className = "SceneStrobeAnimator";
const char* SceneStrobeAnimator::animationName = "Strobe";

// ----------------------------------------------------------------------------
//
SceneStrobeAnimator::SceneStrobeAnimator( UID animation_uid, 
                                          AnimationSignal signal,
                                          UIDArray actors,
                                          RGBWA strobe_neg_color,
                                          unsigned strobe_pos_ms,
                                          unsigned strobe_neg_ms ) :
    SceneColorFader( animation_uid, signal, actors, strobe_neg_color, strobe_pos_ms, strobe_neg_ms, RGBWAArray(), FaderEffect::FADER_EFFECT_STROBE )
{
}

// ----------------------------------------------------------------------------
//
SceneStrobeAnimator::~SceneStrobeAnimator(void)
{
}

// ----------------------------------------------------------------------------
//
AbstractAnimation* SceneStrobeAnimator::clone() {
    return new SceneStrobeAnimator( m_uid, m_signal, m_actors, m_strobe_neg_color, 
                                     m_strobe_pos_ms, m_strobe_neg_ms );
}

// ----------------------------------------------------------------------------
//
CString SceneStrobeAnimator::getSynopsis(void) {
    CString synopsis;

    synopsis.AppendFormat( "Strobe( -color=%s +ms=%u -ms=%u )\n%s", m_strobe_neg_color.getColorName(),
        m_strobe_pos_ms, m_strobe_neg_ms, 
        AbstractAnimation::getSynopsis() );

    return synopsis;
}

// ----------------------------------------------------------------------------
//
void SceneStrobeAnimator::initAnimation( AnimationTask* task, DWORD time_ms, BYTE* dmx_packet ) {
    SceneColorFader::initAnimation( task, time_ms, dmx_packet );

    m_strobe.start( time_ms, m_strobe_pos_ms, m_strobe_neg_ms );
}

// ----------------------------------------------------------------------------
//
bool SceneStrobeAnimator::sliceAnimation( DWORD time_ms, BYTE* dmx_packet )
{
    if ( m_strobe.strobe( time_ms ) ) {
        for ( FaderFixture& f : m_fixtures )
            loadColorChannels( dmx_packet, f, m_strobe.isOn() ? f.m_color : m_strobe.getNegative() );
        return true;
    }

    return false;
}
