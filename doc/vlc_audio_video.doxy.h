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
 * One multimedia stream is associated with each call to @c movie or
 * @c movie_texture. As such, it is possible to play several audio or video
 * files simultaneously, or play files in sequence.
 *
 * The following example shows how you can control
 * the playback through a common function, @c play. In this example, there
 * is only one call to @c movie_texture, in the @c play function.
 * So, there can be only one video
 * playing at any time. By calling @c play with different file names at
 * different times, you can switch between videos. Passing a empty string
 * stops the playback.
 *
 * @code
play T:text ->
    color "white"
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
 * It is currently not possible to start playing at a specified time, or to
 * select the audio or video stream.
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
 */
movie_texture(name:text);

/**
 * @}
 */
