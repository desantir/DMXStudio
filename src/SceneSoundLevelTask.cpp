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

#include "SceneSoundLevelTask.h"

// ----------------------------------------------------------------------------
//
SceneSoundLevelTask::SceneSoundLevelTask( AnimationEngine* engine, UID animation_uid, ActorList& actors, UID owner_uid ) :
    ChannelAnimatorTask( engine, animation_uid, actors, owner_uid )
{}

// ----------------------------------------------------------------------------
//
SceneSoundLevelTask::~SceneSoundLevelTask(void)
{
}

// ----------------------------------------------------------------------------
//
void SceneSoundLevelTask::generateProgram( AnimationDefinition* definition ) 
{
    ChannelValueArray unused_value_array;

    SceneSoundLevel* config = dynamic_cast<SceneSoundLevel *>( definition );

    WORD fade_what = config->getFadeWhat();

    // Determine which channel will be participating
    for ( SceneActor& actor : getActors() ) {
        Fixture* pf = getActorRepresentative( actor.getActorUID() );
        if ( !pf )
            continue;

        // Determine which channels will be participating
        for ( size_t channel=0; channel < pf->getNumChannels(); channel++ ) {
            Channel* cp = pf->getChannel( channel );

            if ( ((fade_what & FADE_COLORS) && cp->isColor()) ||
                ((fade_what & FADE_DIMMERS) && cp->isDimmer()) ) {
                add( actor.getActorUID(), channel, CAM_SCALE, unused_value_array );
            }
        }
    }
}
