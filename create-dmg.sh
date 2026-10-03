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
