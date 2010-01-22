## Spiceweasel automake input. Process with automake to produce Makefile.in


## Set dependencies for the main program

bin_PROGRAMS = spiceweasel
spiceweasel_SOURCES = spiceweasel.c io_png.c io_bmp.c process_frames.c read_main.c io_ipx.c process_script.c parse_nextline.c run_script.c

## Spiceweasel Processing Scripts

spsdir = $(datarootdir)/@PACKAGE@
sps_DATA = scripts/default.sps scripts/example.sps scripts/pass.sps scripts/usharp.sps
AM_CPPFLAGS = -DDEFAULT_SPS_PATH=\"$(spsdir)\"

## Spiceweasel man page

man_MANS = spiceweasel.1
