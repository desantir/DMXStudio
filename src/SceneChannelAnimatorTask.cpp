#include "SceneChannelAnimatorTask.h"
#include "SceneChannelAnimator.h"

// ----------------------------------------------------------------------------
//
SceneChannelAnimatorTask::SceneChannelAnimatorTask( AnimationEngine* engine, UID animation_uid, ActorList& actors, UID owner_uid ) :
    ChannelAnimatorTask( engine, animation_uid, actors, owner_uid )
{
}

// ----------------------------------------------------------------------------
//
SceneChannelAnimatorTask::~SceneChannelAnimatorTask()
{
}

// ----------------------------------------------------------------------------
//
void SceneChannelAnimatorTask::generateProgram( AnimationDefinition* definition )
{
	for ( SceneActor& actor : getActors() ) {
        for ( ChannelAnimation& chan_anim : dynamic_cast<SceneChannelAnimator *>( definition )->channelAnimations() )
            add( actor.getActorUID(), chan_anim.getChannel(), chan_anim.getAnimationStyle(), chan_anim.getChannelValues() );
    }
}