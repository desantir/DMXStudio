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

// NOTE: This is DEPRECATED - use fixture FUID hash instead unless there is a collision


#define MAKE_FUID( manu, model ) ((FUID)(((DWORD)manu << 16) | model ))

#pragma once

#define FW_BLIZZARD				1
#define		FM_PUCK					1
#define		FM_ROCKER_PANEL			2
#define		FM_MIN_WASH				3
#define		FM_MIN_SPOT				4

#define FW_CHAUVET				2
#define		FM_VUE_1_1				1
#define		FM_SWARM_4				2
#define		FM_ABYSS				3
#define		FM_DMX_4_LED			4
#define     FM_PIX_PAR_12_9CH       5
#define     FM_PIX_PAR_12_12CH      6
#define     FM_PIX_PAR_12_3CH       7
#define     FM_PIX_PAR_12_36CH      8

#define FW_AMERICANDJ			3
#define		FM_UVLED_BAR16			1

#define FW_ELATION			    4
