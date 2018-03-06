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

#pragma once

#include "stdafx.h"
#include "Venue.h"
#include "OpenDMXDriver.h"
#include "MusicPlayer.h"
#include "Fixture.h"
#include "SoundSampler.h"
#include "BeatDetector.h"
#include "SoundDetector.h"
#include "TextIO.h"
#include "AudioVolumeController.h"
#include "DMXTextForms.h"

typedef AnimationDefinition* (DMXTextUI::*AnimHandlerEditor)( AnimationDefinition* current_animation, UID reference_fixture_uid );

struct AnimationEditor
{
    CString			    m_name;
    CString			    m_classname;
    AnimHandlerEditor	m_handler;

    AnimationEditor( LPCSTR name, LPCSTR classname, AnimHandlerEditor handler ) :
        m_name( name ),
        m_classname( classname ),
        m_handler( handler )
    {}
};

typedef std::vector<AnimationEditor> AnimationEditorArray;

class DMXTextUI
{
    bool            m_running;
    TextIO	        m_text_io;
    
public:
    DMXTextUI(void);
    ~DMXTextUI(void);

    void run(void);

    static AnimationEditorArray animEditors;

private:
    inline Venue* getVenue() {
        Venue* venue = studio.getVenue();
        STUDIO_ASSERT( venue, "Missing venue" );
        return venue;
    }

    bool isUserSure(void);

    // Maintainence commands
    void help(void);
    void quit(void);
    void loadVenue(void);
    void writeVenue(void);
    void configVenue(void);
    void masterVolume(void);
    void muteVolume(void);

    void blackout(void);
    void autoBlackout(void);
    void soundDebug(void);
    void soundBPM(void);
    void soundDetect(void);
    void soundBeat(void);
    void soundDb();
    void whiteout(void);
    void whiteout_options(void);

    // Fixture control
    void controlFixture(void);
    void describeFixture(void);
    void copyFixture(void);
    void resetDefaultFixtures(void);
    void masterDimmer(void);
    void createFixture(void);
    void deleteFixture(void);
    void updateFixture(void);

    // Scene control
    void createScene(void);
    void selectScene(void);
    void deleteScene(void);
    void updateScene(void);
    void describeScene(void);
    void deleteFixtureFromScene(void);
    void sceneAddAnimation(void);
    void sceneDeleteAllAnimations(void);
    void sceneDeleteAnimation(void);
    void sceneUpdateAnimation(void);
    void copyScene(void);
    void copyWorkspaceToScene(void);

    // Chase control
    void selectChase(void);
    void deleteChase(void);
    void createChase(void);
    void describeChase(void);
    void updateChase(void);
    void copyChaseSteps(void);
    void deleteChaseSteps(void);
    void addChaseSteps(void);
    bool editChaseSteps( Chase* chase, bool append_steps, UINT step_num_offset );
    void advanceChase(void);

    // Palette control
    void describePalette(void); 
    void createPalette(void);
    void deletePalette(void);
    void updatePalette(void);
    void describePaletteEntry( FixtureDefinition* def, PaletteEntry& entry );
    void editPalette( Palette& old_palette, bool create );
    void updatePaletteFixture(void);
    void deletePaletteFixture(void);
    void updatePaletteFixtureDef(void);
    void deletePaletteFixtureDef(void);
    void updateScenePalettes(void);

    // Fixture group control
    void createFixtureGroup(void);
    void describeFixtureGroup(void);
    void deleteFixtureGroup(void);

    // Music control
    void listPlaylists(void);
    void listTracks(void);
    void playTrack(void);
    void queueTrack(void);
    void queuePlaylist(void);
    void playPlaylist(void);
    void pauseTrack(void);
    void stopTrack(void);
    void forwardTrack(void);
    void backTrack(void);
    void showQueuedTracks(void);
    void selectPlaylist( bool queue );
    void selectTrack( bool queue );
    void musicPlayerLogin(void);
    void musicSceneSelect(void);
    void musicMapTrack(void);
    void musicRemoveMapping(void);
    void musicMapShow(void);

    // Animation control
    void animationSpeed( void );
    void createAnimation(void);
    void describeAnimation(void);
    void deleteAnimation(void);
    void updateAnimation(void);

    void test( void );

    void initializeAnimationEditors();

    // Animation editors
    AnimationDefinition* animSequencerEditor( AnimationDefinition* current_animation, UID reference_fixture_uid );
    AnimationDefinition* animSoundLevelEditor( AnimationDefinition* current_animation, UID reference_fixture_uid );
    AnimationDefinition* animPatternDimmerEditor( AnimationDefinition* current_animation, UID reference_fixture_uid );
    AnimationDefinition* animChannelEditor( AnimationDefinition* current_animation, UID reference_fixture_uid );
    AnimationDefinition* animColorFaderEditor( AnimationDefinition* current_animation, UID reference_fixture_uid );
    AnimationDefinition* animMovementEditor( AnimationDefinition* current_animation, UID reference_fixture_uid );
    AnimationDefinition* animStrobeEditor( AnimationDefinition* current_animation, UID reference_fixture_uid );
    AnimationDefinition* animPixelEditor( AnimationDefinition* current_animation, UID reference_fixture_uid );
    AnimationDefinition* animFilterEditor( AnimationDefinition* current_animation, UID reference_fixture_uid );
    AnimationDefinition* animPulseEditor( AnimationDefinition* current_animation, UID reference_fixture_uid );
    AnimationDefinition* animCueEditor( AnimationDefinition* current_animation, UID reference_fixture_uid );
    AnimationDefinition* animFixtureDimmerEditor( AnimationDefinition* current_animation, UID reference_fixture_uid );
};

typedef void (DMXTextUI::*HandlerFunc)();

struct HandlerInfo
{
    HandlerFunc		m_funcptr;
    bool			m_running;
    const char*		m_desc;

    HandlerInfo( HandlerFunc funcptr, bool running, const char* desc ) :
        m_funcptr( funcptr ),
        m_running( running ),
        m_desc( desc)
    {}

    HandlerInfo( ) {}
};

typedef std::map<CString, HandlerInfo> HandlerMap;

class AnimationTypeSelectField : public NumberedListField
{
public:
    AnimationTypeSelectField( LPCSTR label );

    AnimationEditor* getAnimationEditor( ) {
        return &DMXTextUI::animEditors[ getListValue()-1 ]; 
    }
};