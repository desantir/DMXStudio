/* 
Copyright (C) 2016 Robert DeSantis
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

#include "Fixture.h"
#include "AbstractAnimation.h"
#include "ColorFader.h"
#include "SceneActor.h"

struct FadeDelta
{
    channel_t   m_dmx_address;              // This is the resolved multi-universe address
    int         m_step;                     // Step (1 or -1)
    ULONG       m_delta_ms;                 // MS between increments
    ULONG       m_next_ms;                  // MS of next change

    FadeDelta( channel_t dmx_address, int step, LONG delta_ms, ULONG next_ms ) :
        m_dmx_address( dmx_address ),
        m_step( step ),
        m_delta_ms( delta_ms ),
        m_next_ms( next_ms )
    {}

    ~FadeDelta() {}
};

typedef std::vector<FadeDelta> FadeDeltaList;

class ChaseFader : public AbstractAnimation
{
    ULONG           m_fade_time;
    ActorPtrArray   m_target_actors;
    FadeDeltaList   m_fades;

public:
    ChaseFader( UID animation_uid, ULONG fade_time, ActorPtrArray& target_actors );

    virtual ~ChaseFader(void);

    AbstractAnimation* clone() { return NULL; };
    CString getSynopsis(void) { CString dummy; return dummy; }
    const char* getName() { return NULL; }
    const char* getClassName() { return NULL; }
    void accept( IVisitor* visitor) { }

    virtual void initAnimation( AnimationTask* task, DWORD time_ms, BYTE* dmx_packet );
    virtual bool sliceAnimation( DWORD time_ms, BYTE* dmx_packet );
    virtual void stopAnimation( void );
};

