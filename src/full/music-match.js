/* 
Copyright (C) 2012-17 Robert DeSantis
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

// ----------------------------------------------------------------------------
//
function showMusicMatch(event) {
    stopEventPropagation(event);

    $.ajax({
        type: "GET",
        url: "/dmxstudio/rest/query/music/matcher/",
        cache: false,
        success: function (data) {
            var json = jQuery.parseJSON(data);
            $.each(json, function (entry) { entry.isnew = false; });
            musicMatchDialog(json);
        },
        error: onAjaxError
    });
}

// ----------------------------------------------------------------------------
//
function musicMatchDialog( selectionMap ) {
    var send_update = function ( ) {
        $.ajax({
            type: "POST",
            url: "/dmxstudio/rest/edit/music/matcher/load",
            data: JSON.stringify( $("#mmd_mappings").data("selectionMap") ),
            contentType: 'application/json',
            cache: false,
            success: function () {
                ;
            },
            error: onAjaxError
        });

        $("#music_match_dialog").dialog("close");
    };

    $("#music_match_dialog").dialog({
        title: "Music Match Mappings",
        autoOpen: false,
        width: 1048,
        height: 620,
        modal: true,
        resizable: false,
        open: function () { // Stop main body scroll
            // $("body").css("overflow", "hidden");
        },
        close: function () {
            // $("body").css("overflow", "auto");
        },
        buttons: {
            'Save': send_update,
            'Cancel': function () {
                $(this).dialog("close");
            }
        }
    });

    $("#mmd_add").button().click( music_map_add );
    $("#mmd_add_all").button().click( music_map_add_all );

    $("#mmd_add").button("disable");
    $("#mmd_add_all").button("disable");

    setupMusicMatchTrackSelect($("#mmd_playlist_list"), $("#mmd_track_list"));

    $("#mmd_mappings").data("selectionMap", jQuery.extend(true, [], selectionMap));

    populate_music_mappings();

    $("#music_match_dialog").dialog("open");
}

// ----------------------------------------------------------------------------
//
function setupMusicMatchTrackSelect(playlist_select, tracklist_select) {
    function selectPlaylist(tracklist_select, playlist_link) {
        $("#mmd_add_all").button("enable");

        $.ajax({
            type: "GET",
            url: "/dmxstudio/rest/query/music/playlist/tracks/" + playlist_link,
            cache: false,
            success: function (data) {
                var json = jQuery.parseJSON(data);

                tracklist_select.empty();

                $.each(json['tracks'], function (index, track) {
                    tracklist_select.append($('<option>', {
                        value: track.link,
                        text: track.full_name.substring(0, 200),
                        selected: index == 0
                    }));
                });

                tracklist_select.multiselect("refresh");
                tracklist_select.multiselect("uncheckAll");
            },
            error: onAjaxError
        });
    }

    playlist_select.multiselect({
        minWidth: 400, multiple: false, selectedList: 1, header: "Play Lists", noneSelectedText: 'select playlist', classes: 'small_multilist', height: 400
    }).unbind("multiselectclick").bind("multiselectclick", function (event, ui) {
        stopEventPropagation(event);
        selectPlaylist(tracklist_select, ui.value);
    });

    tracklist_select.multiselect({
        minWidth: 400, multiple: false, selectedList: 1, header: "Playlist Tracks", noneSelectedText: 'select track', classes: 'small_multilist', height: 500
    }).unbind("multiselectclick").bind("multiselectclick", function (event, ui) {
        stopEventPropagation(event);
        
        $("#mmd_add").button("enable");
    });;

    // Populate playlists
    $.ajax({
        type: "GET",
        url: "/dmxstudio/rest/query/music/playlists/",
        cache: false,
        success: function (data) {
            var json = jQuery.parseJSON(data);

            playlist_select.empty();
            tracklist_select.empty();

            $.each(json['playlists'], function (index, playlist) {
                playlist_select.append($('<option>', {
                    value: playlist.link,
                    text: playlist.name.substring(0, 200),
                    selected: index == 0
                }));
            });

            playlist_select.multiselect("refresh");
            playlist_select.multiselect("uncheckAll");
            tracklist_select.multiselect("refresh");

            // Populate tracks for selected playlist
            var playlist_link = playlist_select.val();
            if (playlist_link != null && playlist_link > 0)
                selectPlaylist(tracklist_select, playlist_link);
        },
        error: onAjaxError
    });
}

var mm_types = ["Invalid", "Scene", "Chase", "Random scene", "Random chase", "Random scene (BPM)", "Do Nothing" ];
var current_edit = null;

// ----------------------------------------------------------------------------
//
function populate_music_mappings() {
    $("#mmd_mappings").empty();
    current_edit = null;

    var template = $("#mmd_map_template")[0].innerHTML;

    var selectionMap = $("#mmd_mappings").data("selectionMap");
    for (var index = 0; index < selectionMap.length; index++) {
        var container_elem = $(template);
        container_elem.addClass(index & 1 ? "mm_odd" : "mm_even");
        var mappings_elem = $("#mmd_mappings").append(container_elem);
        container_elem.data("mapping", selectionMap[index] );
        setTrackLabels(container_elem);
    }
}

// ----------------------------------------------------------------------------
//
function setTrackLabels(container_elem) {
    var label_elem = container_elem.find(".mm_track_label");
    var type_elem = container_elem.find(".mm_type");
    var select_elem = container_elem.find(".mm_select");
    var remove_elem = container_elem.find(".mm_remove");

    var mapping = container_elem.data("mapping");

    container_elem.click(music_map_edit);

    if (true == mapping.isnew)
        label_elem.text("NEW! " + mapping.track);
    else
        label_elem.text(mapping.track);

    function span(text) {
        return "<span style='margin-left: 5px;'>" + escapeForHTML(text) + "</span>"
    }

    type_elem.html( span(mm_types[mapping.type]) );

    if (mapping.type == 1) {
        var scene = getSceneById(mapping.id);
        select_elem.html(span((scene == null) ? "ERROR" : scene.getFullName()));
    }
    else if (mapping.type == 2) {
        var chase = getChaseById(mapping.id);
        select_elem.html(span((chase == null) ? "ERROR" : chase.getName()));
    }
    else
        select_elem.empty();

    if (mapping.special) {
        remove_elem.hide();
        label_elem.addClass('mm_special_track');
    }
    else {
        remove_elem.click(music_map_remove);
        remove_elem.data("mapping", mapping);
    }
}

// ----------------------------------------------------------------------------
//
function music_map_edit(event) {
    stopEventPropagation(event);

    if (current_edit != null) {
        setTrackLabels(current_edit);
        current_edit = null;
    }

    current_edit = $(this);
    current_edit.click(null);

    var type_elem = current_edit.find(".mm_type");
    var select_elem = current_edit.find(".mm_select");

    var mapping = current_edit.data("mapping");

    var type_select = $("<select>");
    for (var i = 1; i < mm_types.length; i++) {
        if ((i == 2 || i== 4) && chases.length == 0)
            continue;

        type_select.append($("<option>", {
            value: i,
            text: mm_types[i],
            selected: mapping.type == i
        }));
    }

    type_elem.empty();
    type_elem.append(type_select);

    type_select.multiselect({
        classes: 'small_multilist',
        minWidth: 150,
        height: "auto",
        multiple: false,
        selectedList: 1,
        header: false
    }).unbind("multiselectclick").bind("multiselectclick", function (event, ui) {
        stopEventPropagation(event);
        mapping.type = ui.value;
        update_music_map_type();
    });

    var id_select = $("<select>");

    select_elem.empty();
    select_elem.append(id_select);

    id_select.multiselect({
        classes: 'small_multilist',
        minWidth: 320,
        height: "auto",
        multiple: false,
        selectedList: 1,
        header: false
    }).unbind("multiselectclick").bind("multiselectclick", function (event, ui) {
        stopEventPropagation(event);
        mapping.id = ui.value;
    });

    update_music_map_type();
}

// ----------------------------------------------------------------------------
//
function update_music_map_type( ) {
    var select_elem = current_edit.find(".mm_select");

    var mapping = current_edit.data("mapping");

    if (mapping.type >= 3) {
        select_elem.hide();
        return;
    }

    var select = select_elem.find("select");

    select.empty();

    if (mapping.type == 1) {
        if (getSceneById(mapping.id) == null)
            mapping.id = getDefaultSceneId();

        $.map(scenes, function (scene, index) {
            select.append($("<option>", {
                value: scene.getId(),
                text: scene.getFullName(),
                selected: scene.getId() == mapping.id
            }));
        });
    }
    else {
        if (getChaseById(mapping.id) == null)
            mapping.id = chases[0].getId();

        $.map(chases, function (chase, index) {
            select.append( $("<option>", {
                value: chase.getId(),
                text: chase.getName(),
                selected: chase.getId() == mapping.id
            }));
        });
    }

    select.multiselect("refresh");
    select_elem.show();
}

// ----------------------------------------------------------------------------
//
function music_map_add(event) {
    stopEventPropagation(event);

    var mapping = { track: $("#mmd_track_list :selected").text(), 'link': $("#mmd_track_list").val(), 'type': 3, 'id': 1, 'isnew': true }

    var selectionMap = $("#mmd_mappings").data("selectionMap");

    for (var i = 0; i < selectionMap.length; i++) {
        if (selectionMap[i].link == mapping.link) {
            mapping.id = selectionMap[i].id;
            mapping.type = selectionMap[i].type;
            mapping.isnew = selectionMap[i].isnew;
            selectionMap.splice(i, 1);
            break;
        }
    }

    selectionMap.splice(0, 0, mapping);

    populate_music_mappings();

    return false;
}

// ----------------------------------------------------------------------------
//
function music_map_add_all(event) {
    stopEventPropagation(event);

    var selectionMap = $("#mmd_mappings").data("selectionMap");
    var insert_pos = 0;

    $("#mmd_track_list option").each(function () {
        var mapping = { 'track': this.text, link: this.value, 'type': 3, 'id': 1, 'isnew': true }
        for (var i = 0; i < selectionMap.length; i++) {
            if (selectionMap[i].link == mapping.link) {
                mapping.id = selectionMap[i].id;
                mapping.type = selectionMap[i].type;
                mapping.isnew = selectionMap[i].isnew;
                selectionMap.splice(i, 1);
                break;
            }
        }

        selectionMap.splice(insert_pos++, 0, mapping);
    });

    populate_music_mappings();

    return false;
}

// ----------------------------------------------------------------------------
//
function music_map_remove(event) {
    stopEventPropagation(event);

    var selectionMap = $("#mmd_mappings").data("selectionMap");
    var mapping = $(this).data("mapping");

    for (var i = 0; i < selectionMap.length; i++) {
        if (selectionMap[i].track == mapping.track) {
            selectionMap.splice(i, 1);
            populate_music_mappings();
            break;
        }
    }
}
