#!/bin/bash
function make_plist() {
# Create information property list file (Info.plist).
echo '<?xml version="1.0" encoding="UTF-8"?>' >$plistfile
echo '<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">' >>$plistfile
echo '<plist version="1.0">' >>$plistfile
echo '<dict>' >>$plistfile
echo '  <key>CFBundleExecutable</key>' >>$plistfile
echo '  <string>'$appname'</string>' >>$plistfile
echo '  <key>CFBundleGetInfoString</key>' >>$plistfile
echo "  <string>Slic3r Copyright (C) 2011-$(date +%Y) Alessandro Ranellucci</string>" >>$plistfile
echo '  <key>CFBundleIconFile</key>' >>$plistfile
echo '  <string>Slic3r.icns</string>' >>$plistfile
echo '  <key>CFBundleName</key>' >>$plistfile
echo '  <string>Slic3r</string>' >>$plistfile
echo '  <key>CFBundleShortVersionString</key>' >>$plistfile
if [ $TAGGED ]; then
    echo "  <string>Slic3r $SLIC3R_BUILD_ID</string>" >>$plistfile
else 
    echo "  <string>Slic3r $SLIC3R_BUILD_ID-$(git rev-parse --short head)</string>" >>$plistfile
fi
echo '  <key>CFBundleIdentifier</key>' >>$plistfile
echo '  <string>org.slic3r.Slic3r</string>' >>$plistfile
echo '  <key>CFBundleInfoDictionaryVersion</key>' >>$plistfile
echo '  <string>6.0</string>' >>$plistfile
echo '  <key>CFBundlePackageType</key>' >>$plistfile
echo '  <string>APPL</string>' >>$plistfile
echo '  <key>CFBundleSignature</key>' >>$plistfile
echo '  <string>????</string>' >>$plistfile
echo '  <key>CFBundleVersion</key>' >>$plistfile
echo "  <string>${SLIC3R_BUILD_ID}</string>" >>$plistfile
echo '  <key>CGDisableCoalescedUpdates</key>' >>$plistfile
echo '  <false/>' >>$plistfile
echo '</dict>' >>$plistfile
echo '</plist>' >>$plistfile

}
