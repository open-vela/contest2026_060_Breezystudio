if(CONFIG_CONTEST_MUSICPLAYER)
  nuttx_add_application(
    NAME musicplayer
    SRCS music_player.c music_playback.c music_bluetooth.c
    PRIORITY ${CONFIG_CONTEST_MUSICPLAYER_PRIORITY}
    STACKSIZE ${CONFIG_CONTEST_MUSICPLAYER_STACKSIZE})
endif()
