/* Process entry points (main.c, app_modes.c, per-OS main wrappers). */
#ifndef WYE_APP_ENTRY_H
#define WYE_APP_ENTRY_H

/* Runs Wye; argv in UTF-8. */
int wye_main(int argc, char **argv);

/*
 * Internal non-GUI modes; returns their exit code, or -1 for a normal launch.
 *   --help, --version              print usage / version and exit
 *   --install-permissions          Linux, run as root by pkexec from the GUI
 *   --release-keygen <dir> <name>  write a new <name>.key and <name>.pub
 *   --release-sign <key> <file>    write <file>.sig (release automation)
 */
int wye_special_mode(int argc, char **argv);

#endif
