# contest2026_060_Breezystudio/app/launcher - CMakeLists.txt
if(CONFIG_CONTEST_LAUNCHER)
  nuttx_add_application(
    NAME launcher
    SRCS launcher_main.c launcher_ui.c launcher_app.c launcher_manifest.c launcher_player.c launcher_files.c
    PRIORITY ${CONFIG_CONTEST_LAUNCHER_PRIORITY}
    STACKSIZE ${CONFIG_CONTEST_LAUNCHER_STACKSIZE})
endif()
