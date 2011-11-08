/*
 * Documentation for the VLCAudioVideo module.
 *
 * Note: Currently, this file is used only when building the module
 * from the (private) Tao Presentation build environment.
 *
 * (C) 2011 Taodyne SAS <contact@taodyne.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 */

/**
 * @addtogroup VLCAudioVideo VLC audio and video playback
 * @ingroup Modules
 *
 * Play audio and video content. The @c VLCAudioVideo module is
 * based on the
 * <a href="http://www.videolan.org/vlc/">VideoLAN (VLC) player</a>,
 * a free and open source cross-platform multimedia framework that
 * plays most multimedia files as well as DVD, Audio CD, VCD, and
 * various streaming protocols.
 *
 * You can use the commands documented on this page to add music and videos
 * to your presentations. You can even embbed YouTube videos, for example:
 *
 * @code
import VLCAudioVideo
movie "http://www.youtube.com/watch?v=jNQXAC9IVRw"
 * @endcode
 * @image html YouTube.png "Playing a YouTube video in a Tao document"
 *
 * Multimedia streams are identified by their name. It is possible to
 * play several audio or video files simultaneously, or play files in
 * sequence. Use @ref movie_only to ensure that a single video is
 * playing, or @ref movie_drop to drop playback of other videos.
 *
 * The following example shows how you can control
 * the playback through a common function, @c play. In this example, the
 * @c play function calls @ref movie_only, so that a single video plays
 * at any given time.
 * By calling @c play with different file names at different times,
 * you can switch between videos. Passing a empty string stops the
 * playback.
 *
 * @code
import VLCAudioVideo

play T:text ->
    color "white"
    movie_only T
    movie_texture T

page "First video",
    play "first.mpeg"
    rounded_rectangle 0, 0, 352, 240, 20

page "Second video",
    play "second.mpeg"
    rounded_rectangle mouse_x, mouse_y, 352, 240, 20

page "The End",
    text_box 0, 0, window_width, window_height,
        vertical_align_center
        align_center
        font "Tangerine", 72
        color "Black"
        text "That's all Folks!"
    play ""
 * @endcode
 *
 * To start playing at a given time in the movie, use @ref
 * movie_set_time or @ref movie_set_position. To set the movie volume,
 * use @ref movie_set_volume.  To set the movie playback rate, use
 * @ref movie_set_rate. A movie can be paused with @ref movie_pause
 * and resumed with @ref movie_play.
 * 
 * @{
 */

/**
 * Plays audio and/or video.
 * This function creates a rectangular video display centered at @p x, @p y
 * in the plane z = 0. The size of the rectangle depends on the resolution
 * of the video source, and the scaling factors @p sx and @p sy. Set @p sx and
 * @p sy to 1.0 to keep the original resolution.
 * If @p name contains no video, nothing is displayed (only the audio track
 * will be played, if present).
 * The @p name parameter specifies a local file or a URL; an empty string
 * @c "" stops playback of the current video.
 * This function is based on @ref movie_texture.
 * @see movie_texture.
 */
movie(x:real, y:real, sx:real, sy:real, name:text);

/**
 * Plays audio and/or video.
 * This function is equivalent to <tt>movie 0, 0, 1, 1, name</tt>.
 * @see movie_texture.
 */
movie(name:text);

/**
 * Creates a video texture.
 * This primitive plays an audio and/or video file and makes the video
 * available as a texture. You can subsequently map the texture on a rectangle
 * or any other shape in the 3D space.
 * The @p name parameter specifies a local file or a URL; an empty string
 * @c "" stops playback of the current video.
 * If @p name refers to a video, the resolution is made available through
 * the @ref texture_width and @ref texture_height primitives. No texture
 * is bound when @p name contains no video, or until the first frame is
 * available.
 * When the end of the media stream is reached, playback stops and the last
 * frame remains available as a texture.
 * @see movie.
 */
movie_texture(name:text);

/**
 * Drop all references to a video stream.
 * This command stops the given video and releases any resource used to play it.
 * The @p name parameter specifies the name of the movie.
 * @see movie_only.
 */
movie_drop(name:text);

/**
 * Keep only one active movie.
 * This command stops and drops all video/audio streams but the selected one.
 * It is similar to calling @ref movie_drop for all currently active streams
 * except the current one.
 * The @p name parameter specifies the name of the movie.
 * @see movie_drop.
 */
movie_only(name:text);

/**
 * Resume playback of a paused movie.
 * This command plays a movie that was paused by @ref movie_pause.
 * The @p name parameter specifies the name of the movie.
 * @see movie_pause, movie_stop.
 */
movie_play(name:text);

/**
 * Pause movie playback.
 * This command pauses a movie at its current location. The movie playback
 * can be resumed using @ref movie_play.
 * The @p name parameter specifies the name of the movie.
 * @see movie_play, movie_pause.
 */
movie_pause(name:text);

/**
 * Stop movie playback.
 * This command stops movie playback.
 * The @p name parameter specifies the name of the movie.
 * @see movie_play, movie_pause.
 */
movie_stop(name:text);

/**
 * Return the playback volume for a given movie.
 * The movie volume is returned in the range 0.0 (muted) to 1.0 (maximum volume)
 * The @p name parameter specifies the name of the movie.
 * @see movie_set_volume.
 * Use @ref movie_set_volume to set the volume.
 */
movie_volume(name:text);

/**
 * Return the position in a given movie as a percentage.
 * The movie position is returned in the range 0.0 (start of the movie)
 * to 1.0 (end of the movie)
 * The @p name parameter specifies the name of the movie.
 * @see movie_set_position.
 * Use @ref movie_set_position to set the position.
 */
movie_position(name:text);

/**
 * Return the position in a given movie in seconds from the start.
 * The movie position is returned in seconds from the start of the movie.
 * The @p name parameter specifies the name of the movie.
 * @see movie_set_time.
 * Use @ref movie_set_time to set the position in the movie by time.
 */
movie_time(name:text);

/**
 * Return the length of a given movie in seconds.
 * The @p name parameter specifies the name of the movie.
 * @see movie_time, movie_set_time.
 */
movie_length(name:text);

/**
 * Return the playback rate of the given movie.
 * Return the movie playback rate, where 1.0 is normal playback rate,
 * 2.0 indicates the movie is played twice faster, and 0.5 indicates that
 * it's being played at half-speed.
 * The @p name parameter specifies the name of the movie.
 * @see movie_set_rate.
 * Use @ref movie_set_rate to set the playback rate.
 */
movie_rate(name:text);

/**
 * Returns true if the movie is currently playing.
 * The @p name parameter specifies the name of the movie.
 * @see movie_paused, movie_done.
 */
movie_playing(name:text);

/**
 * Returns true if the movie is currently paused.
 * The @p name parameter specifies the name of the movie.
 * @see movie_playing, movie_done.
 */
movie_paused(name:text);

/**
 * Returns true if the movie is was played until its end.
 * The @p name parameter specifies the name of the movie.
 * @see movie_paused, movie_playing.
 */
movie_done(name:text);


/**
 * Sets the playback volume for the movie.
 * Adjust the sound playback volume for the given movie.
 * The @p name parameter specifies the name of the movie.
 * The @p volume parameter specifies the volume, in the range 0.0 (silent)
 * to 1.0 (maximum volume).
 * @see movie_volume.
 * Use @ref movie_volume to get the current volume.
 */
movie_set_volume(name:text, volume:real);

/**
 * Sets the playback position for the movie.
 * Adjust the movie playback position for the given movie.
 * The @p name parameter specifies the name of the movie.
 * The @p position parameter specifies the position in the movie,
 * in the range 0.0 (beginning of the movie) to 1.0 (end of the movie).
 * @see movie_position, movie_time, movie_set_time.
 * Use @ref movie_position to get the current position.
 */
movie_set_position(name:text, position:real);

/**
 * Sets the playback position in seconds.
 * Adjust the movie playback position for the given movie.
 * The @p name parameter specifies the name of the movie.
 * The @p time parameter specifies the position in the movie in seconds,
 * in the range 0.0 (beginning of the movie) to the @ref movie_length.
 * @see movie_time, movie_position, movie_set_position.
 * Use @ref movie_time to get the current position.
 */
movie_set_time(name:text, time:real);

/**
 * Sets the playback rate.
 * Adjust the movie playback rate for the given movie.
 * The @p name parameter specifies the name of the movie.
 * The @p rate parameter specifies the rate of the movie, where 1.0 is
 * normal playback speed, 2.0 indicates a 2x speedup, and 0.5 indicates
 * half-speed playback.
 * @see movie_rate.
 * Use @ref movie_rate to get the current playback rate.
 */
movie_set_rate(name:text, rate:real);


/**
 * @}
 */
