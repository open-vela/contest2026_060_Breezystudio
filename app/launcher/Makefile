############################################################################
# contest2026_060_Breezystudio/app/launcher/Makefile
#
# Vela Launcher - LVGL-based application launcher.
############################################################################

include $(APPDIR)/Make.defs

PROGNAME  = launcher
PRIORITY  = $(CONFIG_CONTEST_LAUNCHER_PRIORITY)
STACKSIZE = $(CONFIG_CONTEST_LAUNCHER_STACKSIZE)
MODULE    = $(CONFIG_CONTEST_LAUNCHER)

MAINSRC = launcher_main.c
CSRCS = launcher_ui.c launcher_app.c launcher_manifest.c launcher_player.c launcher_files.c

include $(APPDIR)/Application.mk
