#!/bin/bash
#VERSION=$(head -1 debian/changelog | sed 's/.*(\(.*\)).*/\1/')
VERSION=$(grep -m 1 APPVERSION variables.pri | cut -d '=' -f 2 | sed -e 's/^[[:space:]]*//' | tr ' ' _ | tr -d '\r\n')

# Build
if [ -n "$QTDIR" ]; then
    $QTDIR/bin/qmake $1
    make distclean
    # Compile translations
    if [[ $1 == *"qmlui"* ]]; then
        ./translate.sh "qmlui"
    else
        ./translate.sh "ui"
    fi
    $QTDIR/bin/qmake $1
else
    qmake -spec macx-g++
    make distclean
    qmake -spec macx-g++
fi

NUM_CPUS=`sysctl -n hw.ncpu` || true
if [ -z "$NUM_CPUS" ]; then
    NUM_CPUS=4
fi

make -j$NUM_CPUS
rc=$?
if [ $rc -ne 0 ]; then
    echo Compiler error. Aborting package creation.
    exit $rc
fi

# Install to ~/QLC+.app/
make install
rc=$?
if [ $rc -ne 0 ]; then
    echo Installation error. Aborting package creation.
    exit $rc
fi

# Let Qt finish the bundle.
#
# The qmake packaging in this tree rewrites install names itself, but it was
# written for a Qt installer layout: it assumes the frameworks live under
# $QTDIR/lib and already reference each other by paths it can predict. A
# Homebrew Qt does not look like that - the frameworks refer to each other
# through /opt/homebrew/Cellar/qt@5/<version> while everything else refers to
# them through the /opt/homebrew/opt/qt@5 symlink, and Qt itself pulls in
# glib, pcre2, zstd, gettext, libpng, md4c and freetype, which the packaging
# knows nothing about. install_name_tool exits 0 when the path it was told to
# change is not present, so every mismatched rewrite silently did nothing and
# the bundle shipped referring to libraries that only exist on the build
# machine.
#
# macdeployqt walks every binary in the bundle, copies in whatever is still
# missing and rewrites the lot. Run it before building the image.
if [ -n "$QTDIR" ] && [ -x "$QTDIR/bin/macdeployqt" ]; then
    MACDEPLOYQT="$QTDIR/bin/macdeployqt"
else
    MACDEPLOYQT=$(command -v macdeployqt)
fi

if [ -z "$MACDEPLOYQT" ]; then
    echo "macdeployqt not found. Set QTDIR or put it on PATH."
    exit 1
fi

"$MACDEPLOYQT" ~/QLC+.app
rc=$?
if [ $rc -ne 0 ]; then
    echo "macdeployqt failed. Aborting package creation."
    exit $rc
fi

# Create Apple Disk iMaGe from ~/QLC+.app/
OUTDIR=$PWD
cd platforms/macos/dmg
./create-dmg --volname "Q Light Controller Plus $VERSION" \
    --volicon $OUTDIR/resources/icons/qlcplus.icns \
    --background background.png \
    --window-size 400 300 \
    --window-pos 200 100 \
    --icon-size 64 \
    --icon "QLC+" 0 150 \
    --app-drop-link 200 150 \
    $OUTDIR/QLC+_$VERSION.dmg \
    ~/QLC+.app
rc=$?
cd -

# Verify we really produced the image. create-dmg can fail partway and leave
# only the writable rw.* intermediate, which previously went unnoticed because
# nothing here checked.
if [ $rc -ne 0 ] || [ ! -f "$OUTDIR/QLC+_$VERSION.dmg" ]; then
    echo "DMG creation failed: $OUTDIR/QLC+_$VERSION.dmg was not produced."
    rm -f "$OUTDIR/rw.QLC+_$VERSION.dmg"
    exit 1
fi
echo "Created $OUTDIR/QLC+_$VERSION.dmg"
