/* 
Copyright (C) 2017-2018 Robert DeSantis
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

#include "Palette.h"

// ----------------------------------------------------------------------------
//
PaletteEntry* Palette::getEntry( Fixture* pf ) {
    PaletteEntry* entry = getFixtureEntry( pf->getUID() );

    if ( entry == NULL ) {
        entry = getFixtureDefinitionEntry( pf->getFUID() );

        if ( entry == NULL && m_global.hasValues() )
            entry = getGlobalEntry();
    }

    return entry;
}

// ----------------------------------------------------------------------------
//
bool WeightedColorCompare( WeightedColor& c1, WeightedColor& c2 ) {
    return c1.getWeight() > c2.getWeight();
}

// ----------------------------------------------------------------------------
//
WeightedColorArray& Palette::getWeightedColors() {
    if ( m_weighted_colors.size() > 0 || m_palette_colors.size() == 0 )
        return m_weighted_colors;

    if ( m_palette_weights.size() != m_palette_colors.size() ) {
        double weight = 1.0 / m_palette_weights.size();

        for ( RGBWA& color : m_palette_colors )
            m_weighted_colors.emplace_back( color, weight );
    
        return m_weighted_colors;
    }

    for ( size_t i=0; i < m_palette_colors.size(); i++ )
        m_weighted_colors.emplace_back( m_palette_colors[i], m_palette_weights[i] );

    // Sort the palette
    std::sort( m_weighted_colors.begin(), m_weighted_colors.end(), WeightedColorCompare );

    return m_weighted_colors;
}