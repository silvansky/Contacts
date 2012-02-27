message(Ignoring plugin \"$${TARGET}\")
# TODO: find a better way to prevent build of a project
QMAKE_CXX=echo
QMAKE_LINK=echo
win32: {
  CONFIG-=embed_manifest_exe
  CONFIG-=embed_manifest_dll
  TARGET=
}
