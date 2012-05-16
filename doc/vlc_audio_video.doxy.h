/*
 * Documentation for the VLCAudioVideo module.
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
 * @~english
 * @taomoduledescription{VLCAudioVideo, VLC audio and video playback}
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
 * you can switch between videos. Passing an empty string stops the
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
 * In compliance with the GNU General Public Licence for the VLC code used
 * by this module, the source code for the module is available here:
 * http://gitorious.org/tao-presentation-modules
 * 
 * @endtaomoduledescription{VLCAudioVideo}
 *
 * @~french
 * @taomoduledescription{VLCAudioVideo, Lecteur Audio/Vidéo VLC}
 *
 * Lecteur multimédia (audio et vidéo) basé sur le lecteur
 * <a href="http://www.videolan.org/vlc/">VideoLAN (VLC)</a>, un outil
 * du logiciel libre, multi-plateforme, qui permet de jouer la plupart des
 * fichiers mutimédia ainsi que les DVD, CD audio, VCD et même les flux
 * réseau.
 *
 * Vous pouvez utiliser les commandes documentées ici pour ajouter de la
 * musique et des vidéos à vos présentations. Vous pouvez même jouer des vidéos
 * YouTube :
 *
 * @code
import VLCAudioVideo
movie "http://www.youtube.com/watch?v=jNQXAC9IVRw"
 * @endcode
 * @image html YouTube.png "Une vidéo YouTube dans un document Tao"
 *
 * Les flux multimédias sont identifiés par leur nom. Il est possible de
 * faire jouer plusieurs fichiers simultanément, ou de les jouer les uns
 * après les autres. Utilisez @ref movie_only pour ne jouer qu'une seule
 * vidéo (et éventuellement arrêter une autre qui serait en train de jouer),
 * ou @ref movie_drop pour arrêter la lecture d'une vidéo donnée.
 *
 * L'exemple qui suit vous montre comment jouer simplement une vidéo par
 * page, en utilisant une fonction commune, @c jouer. Cette fonction appelle
 * @ref movie_only pour s'assurer qu'aucune autre vidéo que celle spécifiée
 * n'est en cours de lecture, puis @ref movie_texture pour lire le fichier
 * demandé.
 * En appelant @c play avec des noms de fichiers différents à différents
 * moments, vous pouvez passer d'une vidéo à l'autre. C'est ce qui se produit
 * lorsque vous changez de page dans l'exemple. Pour stopper la lecture,
 * on passe une chaîne vide à la fonction.
 *
 * @code
import VLCAudioVideo

jouer T:text ->
    color "white"
    movie_only T
    movie_texture T

page "First video",
    jouer "first.mpeg"
    rounded_rectangle 0, 0, 352, 240, 20

page "Second video",
    jouer "second.mpeg"
    rounded_rectangle mouse_x, mouse_y, 352, 240, 20

page "The End",
    text_box 0, 0, window_width, window_height,
        vertical_align_center
        align_center
        font "Tangerine", 72
        color "Black"
        text "That's all Folks!"
    jouer ""
 * @endcode
 *
 * Pour commencer à jouer à une certaine position, utilisez
 * @ref movie_set_time ou @ref movie_set_position. Pour régler le volume,
 * utilisez @ref movie_set_volume. Il est possible d'accélérer ou de
 * ralentir la lecture grâce à @ref movie_set_rate. Pour mettre en pause,
 * appeler @ref movie_pause; @ref movie_play permet de poursuivre la
 * lecture.
 *
 * En accord avec la licence GNU General Public Licence du code VLC utilisé
 * par ce module, le code source du module est disponible ici:
 * http://gitorious.org/tao-presentation-modules
 * 
 * @endtaomoduledescription{VLCAudioVideo}
 * @~
 * @{
 */

/**
 * @~english
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
 * @~french
 * Lit un fichier audio et/ou vidéo.
 * Cette fonction crée un affichage rectangulaire centré en @p x, @p y dans
 * le plan z = 0. La taille du rectangle dépend de la résolution de la vidéo,
 * et des facteurs multiplicatifs @p sx et @p sy. Donnez-leur la valeur 1.0
 * pour conserver la taille d'origine de la vidéo.
 * Si @p name ne contient pas de piste vidéo, rien n'est affiché et seule la
 * piste son sera jouée, si elle existe.
 * Cette fonction utilise @ref movie_texture.
 * @~
 * @see movie_texture.
 */
movie(x:real, y:real, sx:real, sy:real, name:text);

/**
 * @~english
 * Plays audio and/or video.
 * This function is equivalent to :
 * @~french
 * Lit un fichier audio et/ou vidéo.
 * Cette fonction est équivalente à :
 * @~
 * @code movie 0, 0, 1, 1, name @endcode
 * @see movie_texture.
 */
movie(name:text);

/**
 * @~english
 * Creates a video texture.
 * This primitive plays an audio and/or video file and makes the video
 * available as a texture. You can subsequently map the texture on a rectangle
 * or any other shape in the 3D space.
 * The @p name parameter specifies a local file or a URL.
 * If @p name refers to a video, the resolution is made available through
 * the @ref texture_width and @ref texture_height primitives. No texture
 * is bound when @p name contains no video, or until the first frame is
 * available.
 * When the end of the media stream is reached, playback stops and the last
 * frame remains available as a texture.
 * Since version 1.03, it is possible to append media-specific VLC options to
 * the @p name. The separator is <tt>##</tt>. Several options may be specified,
 * separated with spaces. For instance:
 * @code
movie_texture "video.mp4##input-repeat=1 start-time=10 stop-time=15"
 * @endcode
 * Refer to the VLC documentation for information on media-specific options.
 * @note Some VLC options have no effect, such as video filters which are
 * currently not useable within Tao Presentations.
 * @~french
 * Crée une texture vidéo.
 * Cette primitive lit un fichier audio et/ou vidéo et rend disponible la
 * vidéo sous forme de texture. Vous pouvez ensuite appliquer cette texture sur
 * un rectangle ou n'importe quelle autre forme dans l'espace 3D.
 * Le paramètre @p name spécifie un fichier local ou une URL. Une chaîne vide
 * permet d'arrêter la lecture de la vidéo associée à cet appel.
 * Lorsque @p name représente une vidéo, la résolution de l'image est
 * disponible grâce aux primitives @ref texture_width et @ref texture_height.
 * Si par contre @p name ne contient pas de vidéo, ou tant que la lecture n'est
 * pas commencée, aucune texture n'est activée.
 * Lorsque la fin du fichier est atteinte, la lecture s'arrête et la dernière
 * image reste disponible dans la texture.
 * Depuis la version 1.03, il est possible d'ajouter des options VLC spécifiques
 * au média grâce au paramètre @p name. Le séparateur est <tt>##</tt>. Plusieurs
 * options peuvent être séparées par des espaces.
 * Par exemple :
 * @code
movie_texture "video.mp4##input-repeat=1 start-time=10 stop-time=15"
 * @endcode
 * Voyez la documentation VLC pour plus d'informations sur les options média
 * de VLC.
 * @note Certaines options n'ont aucun effet, comme par exemple les filtres
 * vidéo qui ne sont pas actuellement utilisables dans Tao Presentations.
 * @~
 * @see movie.
 */
movie_texture(name:text);

/**
 * @~english
 * Creates a video texture of a specific size.
 * This function is similar to @ref movie_texture(name:text), except that
 * instead of creating a texture of the same size as the video, it uses a
 * specific size.
 * Note that the @p width and @p height parameters are
 * ignored if the video has already been started, even after @ref movie_stop.
 * You need to delete the movie player instance
 * (@ref movie_drop, @ref movie_only) to play the video again with a different
 * resolution.
 * This function will start playback faster than the version without
 * @p width and @p height, especially when playing a network stream.
 * @~french
 * Crée une texture vidéo de la taille spécifiée.
 * Cette fonction est similaire à @ref movie_texture(name:text), mais au lieu
 * de créer une texture de la même taille que la vidéo, elle permet de préciser
 * une taille. Notez que @p width et @p height ne sont pas utilisés si la
 * vidéo a déjà été démarrée, et même après @ref movie_stop. Il faut détruire
 * l'instance du lecteur multimédia (@ref movie_drop, @ref movie_only) pour
 * pouvoir redémarrer la lecture avec une résolution différente.
 * Cette fonction démarre la lecture plus rapidement que la version sans
 * @p width et @p height, surtout lorsque @p name représente un flux réseau.
 * @~
 * @see movie_texture(name:text), movie_drop, movie_only
 * @since 1.01
 */
movie_texture(name:text, width:integer, height:integer);

/**
 * @~english
 * Plays video in the Tao window with best performance.
 * A new graphical widget is created and covers the whole Tao Presentations
 * window. The video is rendered directly into this widget by libVLC.
 * The video is always sized to fit the window (which may or may not be in
 * fullscreen mode).
 * @n
 * When the movie ends, or the movie is stopped (see @ref movie_stop,
 * @ref movie_drop, @ref movie_only), the widget is destroyed and the Tao
 * document is visible again.
 * @n
 * Playing a video with this method is usually more efficient than using
 * @ref movie or @ref movie_texture, because it allows libVLC to enable
 * additional optimizations.
 * @~french
 * Lit un fichier vidéo dans la fenêtre Tao avec les meilleurs performances.
 * Une nouvelle zone graphique est créée par dessus celle de Tao Presentations et
 * la couvre entièrement. La vidéo est affichée directement dans cette
 * zone par libVLC. La vidéo est toujours affichée de telle manière à remplir
 * la fenêtre, que celle-ci soit ou non en mode plein écran.
 * @n
 * Lorsque le clip se termine, ou lorsqu'il est stoppé
 * (cf. @ref movie_stop, @ref movie_drop, @ref movie_only), la zone graphique
 * est détruite.
 * @n
 * La lecture d'une vidéo par cette méthode est généralement plus efficace
 * qu'avec @ref movie ou @ref movie_texture, car elle permet à libVLC de
 * faire des optimisations supplémentaires.
 * @~
 * @since 1.04
 */
movie_fullscreen(name:text);


/**
 * @~english
 * Drop all references to a video stream.
 * This command stops the given video and releases any resource used to play it.
 * The @p name parameter specifies the name of the movie.
 * @~french
 * Annule toute lecture d'un fichier donné.
 * Cette commande arrête la lecture du fichier spécifié par @p name, et libère
 * toutes les ressources associées.
 * @~
 * @see movie_only.
 */
movie_drop(name:text);

/**
 * @~english
 * Keep only one active movie.
 * This command stops and drops all video/audio streams but the selected one.
 * It is similar to calling @ref movie_drop for all currently active streams
 * except the current one.
 * The @p name parameter specifies the name of the movie.
 * @~french
 * Garde un seul flux multimédia actif.
 * Cette commande arrête la lecture de tous les flux audio/vidéo en cours, sauf
 * @p name. Elle a un effet équivalent à appeler @ref movie_drop pour tous les
 * flux multimédias actifs sauf @p name.
 * @p name est le nom du fichier ou l'URL de la ressource multimédia.
 * @~
 * @see movie_drop.
 */
movie_only(name:text);

/**
 * @~english
 * Resume playback of a paused movie.
 * This command plays a movie that was paused by @ref movie_pause.
 * The @p name parameter specifies the name of the movie.
 * @~french
 * Reprend la lecture d'un flux multimédia.
 * Cette commande reprend la lecture d'un flux qui a été arrêté par
 * @ref movie_pause.
 * @p name est le nom du fichier ou l'URL de la ressource multimédia.
 * @~
 * @see movie_pause, movie_stop.
 */
movie_play(name:text);

/**
 * @~english
 * Pause movie playback.
 * This command pauses a movie at its current location. The movie playback
 * can be resumed using @ref movie_play.
 * The @p name parameter specifies the name of the movie.
 * @~french
 * Suspend la lecture d'un flux multimédia.
 * Cette commande suspend temporairement la lecture du flux @p name. La lecture
 * reprend au même endroit lorsque @ref movie_play est appelé.
 * @p name est le nom du fichier ou l'URL de la ressource multimédia.
 * @~
 * @see movie_play, movie_pause.
 */
movie_pause(name:text);

/**
 * @~english
 * Stop movie playback.
 * This command stops movie playback.
 * The @p name parameter specifies the name of the movie.
 * @~french
 * Arrête la lecture d'un flux multimédia.
 * Cette commande arrête la lecture du flux @p name. Il est possible de
 * relancer la lecture à nouveau en utilisant @ref play_movie, mais la lecture
 * reprendra au début.
 * @p name est le nom du fichier ou l'URL de la ressource multimédia.
 * @~
 * @see movie_play, movie_pause.
 */
movie_stop(name:text);

/**
 * @~english
 * Return the playback volume for a given movie.
 * The movie volume is returned in the range 0.0 (muted) to 1.0 (maximum volume)
 * The @p name parameter specifies the name of the movie.
 * @~french
 * Renvoie le volume audio d'un flux multimédia en cours de lecture.
 * Le volume va de 0.0 (pas de son) à 1.0 (volume maximum).
 * @p name est le nom du fichier ou l'URL de la ressource multimédia.
 * @~
 * @see movie_set_volume.
 */
movie_volume(name:text);

/**
 * @~english
 * Return the position in a given movie as a percentage.
 * The movie position is returned in the range 0.0 (start of the movie)
 * to 1.0 (end of the movie)
 * The @p name parameter specifies the name of the movie.
 * @~french
 * Renvoie la position courante dans un flux multimédia en cours de lecture.
 * La position va de 0.0 (début) à 1.0 (fin).
 * @p name est le nom du fichier ou l'URL de la ressource multimédia.
 * @~
 * @see movie_set_position.
 */
movie_position(name:text);

/**
 * @~english
 * Return the position in a given movie in seconds from the start.
 * The movie position is returned in seconds from the start of the movie.
 * The @p name parameter specifies the name of the movie.
 * @~french
 * Renvoie la position en secondes dans un flux multimédia en cours de lecture.
 * La position est exprimée en secondes depuis le début du fichier.
 * @p name est le nom du fichier ou l'URL de la ressource multimédia.
 * @~
 * @see movie_set_time.
 */
movie_time(name:text);

/**
 * @~english
 * Return the length of a given movie in seconds.
 * The @p name parameter specifies the name of the movie.
 * @~french
 * Renvoie la longueur en secondes d'un flux multimédia en cours de lecture.
 * @p name est le nom du fichier ou l'URL de la ressource multimédia.
 * @~
 * @see movie_time, movie_set_time.
 */
movie_length(name:text);

/**
 * @~english
 * Return the playback rate of the given movie.
 * Return the movie playback rate, where 1.0 is normal playback rate,
 * 2.0 indicates the movie is played twice faster, and 0.5 indicates that
 * it's being played at half-speed.
 * The @p name parameter specifies the name of the movie.
 * @~french
 * Renvoie la vitesse de lecture d'un flux multimédia en cours de lecture.
 * 1.0 est la vitesse normale, 2.0 signifie que le fichier est joué deux fois
 * plus vite que la normale, et 0.5 correspond à un ralenti de moitié.
 * @p name est le nom du fichier ou l'URL de la ressource multimédia.
 * @~
 * @see movie_set_rate.
 */
movie_rate(name:text);

/**
 * @~english
 * Returns true if the movie is currently playing.
 * The @p name parameter specifies the name of the movie.
 * @~french
 * Renvoie true si le flux multimédia est en cours de lecture.
 * @p name est le nom du fichier ou l'URL de la ressource multimédia.
 * @~
 * @see movie_paused, movie_done.
 */
movie_playing(name:text);

/**
 * @~english
 * Returns true if the movie is currently paused.
 * The @p name parameter specifies the name of the movie.
 * @~french
 * Renvoie true si le flux multimédia est en pause.
 * @p name est le nom du fichier ou l'URL de la ressource multimédia.
 * @~
 * @see movie_playing, movie_done.
 */
movie_paused(name:text);

/**
 * @~english
 * Returns true if the movie is was played until its end.
 * The @p name parameter specifies the name of the movie.
 * @~french
 * Renvoie true si le flux multimédia a joué jusqu'au bout.
 * @p name est le nom du fichier ou l'URL de la ressource multimédia.
 * @~
 * @see movie_paused, movie_playing.
 */
movie_done(name:text);

/**
 * @~english
 * Returns true if loop mode is enabled for the movie.
 * The @p name parameter specifies the name of the movie.
 * @~french
 * Renvoie true si le mode boucle est actif pour le flux multimédia.
 * @p name est le nom du fichier ou l'URL de la ressource multimédia.
 * @~
 * @see movie_set_loop.
 */
movie_loop(name:text);


/**
 * @~english
 * Sets the playback volume for the movie.
 * Adjust the sound playback volume for the given movie.
 * The @p name parameter specifies the name of the movie.
 * The @p volume parameter specifies the volume, in the range 0.0 (silent)
 * to 1.0 (maximum volume).
 * @~french
 * Contrôle le volume audio.
 * @p name est le nom du fichier ou l'URL de la ressource multimédia.
 * @~
 * @see movie_volume.
 */
movie_set_volume(name:text, volume:real);

/**
 * @~english
 * Sets the playback position for the movie.
 * Adjust the movie playback position for the given movie.
 * The @p name parameter specifies the name of the movie.
 * The @p position parameter specifies the position in the movie,
 * in the range 0.0 (beginning of the movie) to 1.0 (end of the movie).
 * @~french
 * Choisit la position à l'intérieur d'un flux multimédia.
 * @p name est le nom du fichier ou l'URL de la ressource multimédia.
 * @~
 * @see movie_position, movie_time, movie_set_time.
 */
movie_set_position(name:text, position:real);

/**
 * @~english
 * Sets the playback position in seconds.
 * Adjust the movie playback position for the given movie.
 * The @p name parameter specifies the name of the movie.
 * The @p time parameter specifies the position in the movie in seconds,
 * in the range 0.0 (beginning of the movie) to the @ref movie_length.
 * @~french
 * Choisit la position en secondes à l'intérieur d'un flux multimédia.
 * @p name est le nom du fichier ou l'URL de la ressource multimédia.
 * @~
 * @see movie_time, movie_position, movie_set_position.
 */
movie_set_time(name:text, time:real);

/**
 * @~english
 * Sets the playback rate.
 * Adjust the movie playback rate for the given movie.
 * The @p name parameter specifies the name of the movie.
 * The @p rate parameter specifies the rate of the movie, where 1.0 is
 * normal playback speed, 2.0 indicates a 2x speedup, and 0.5 indicates
 * half-speed playback.
 * @~french
 * Choisit la vitesse de lecture d'un flux multimédia.
 * @p name est le nom du fichier ou l'URL de la ressource multimédia.
 * @~
 * @see movie_rate.
 */
movie_set_rate(name:text, rate:real);


/**
 * @~english
 * Sets the loop mode.
 * Enables or disables loop mode for the given movie.
 * The @p name parameter specifies the name of the movie.
 * If @p mode is true, the movie will automatically be restarted when done
 * playing.
 * @~french
 * Active ou désactive la lecture en boucle d'un flux multimédia.
 * @p name est le nom du fichier ou l'URL de la ressource multimédia.
 * Si @p mode est @a true, la lecture reprend automatiquement dès que la fin
 * de la lecture est atteinte.
 * @~
 * @see movie_loop.
 */
movie_set_loop(name:text, mode:boolean);


/**
 * @~english
 * Initializes the VLC library.
 * With this function you can pass specific command-line options to VLC.
 * For example:
 * @~french
 * Initialise la bibliothèque VLC.
 * Cette fonction permet de passer des options spécifiques.
 * Par exemple :
 * @~
 * @code
import VLCAudioVideo 1.02
vlc_init "--http-proxy", "http://proxy.example.com:8080"
movie "http://youtube.com/watch?v=bKpPTq3-Qic"
 * @endcode
 * @since 1.02
 */
vlc_init(options:tree);

/**
 * @}
 */
