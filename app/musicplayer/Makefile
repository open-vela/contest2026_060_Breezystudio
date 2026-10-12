############################################################################
# Music Player - Makefile
############################################################################
include $(APPDIR)/Make.defs

PROGNAME  = musicplayer
PRIORITY  = $(CONFIG_CONTEST_MUSICPLAYER_PRIORITY)
STACKSIZE = $(CONFIG_CONTEST_MUSICPLAYER_STACKSIZE)
MODULE    = $(CONFIG_CONTEST_MUSICPLAYER)

MAINSRC = music_player.c
CSRCS = music_playback.c music_bluetooth.c

include $(APPDIR)/Application.mk
