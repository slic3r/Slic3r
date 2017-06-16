#!/bin/bash
function make_plist() {
# Create information property list file (Info.plist).

cat << EOF > $plistfile
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
  <key>CFBundleExecutable</key>
  <string>$appname</string>
  <key>CFBundleGetInfoString</key>
  <string>Slic3r Copyright (C) 2011-$(date +%Y) Alessandro Ranellucci</string>
  <key>CFBundleIconFile</key>
  <string>Slic3r.icns</string>
  <key>CFBundleName</key>
  <string>Slic3r</string>
  <key>CFBundleShortVersionString</key>
EOF

if [ $TAGGED ]; then
    echo "  <string>Slic3r $SLIC3R_BUILD_ID</string>" >>$plistfile
else
    echo "  <string>Slic3r $SLIC3R_BUILD_ID-$(git rev-parse --short head)</string>" >>$plistfile
fi

cat << EOF >> $plistfile
  <key>CFBundleIdentifier</key>
  <string>org.slic3r.$appname</string>
  <key>CFBundleInfoDictionaryVersion</key>
  <string>6.0</string>
  <key>CFBundlePackageType</key>
  <string>APPL</string>
  <key>CFBundleSignature</key>
  <string>????</string>
  <key>CFBundleVersion</key>
  <string>${SLIC3R_BUILD_ID}</string>
  <key>CFBundleDocumentTypes</key>
	<array>
		<dict>
			<key>CFBundleTypeExtensions</key>
			<array>
				<string>stl</string>
				<string>STL</string>
			</array>
			<key>CFBundleTypeIconFile</key>
			<string>Slic3r.icns</string>
			<key>CFBundleTypeName</key>
			<string>STL</string>
			<key>CFBundleTypeRole</key>
			<string>Viewer</string>
            <key>LISsAppleDefaultForType</key>
            <true/>
            <key>LSHandlerRank</key>
            <string>Alternate</string>
		</dict>
		<dict>
			<key>CFBundleTypeExtensions</key>
			<array>
				<string>obj</string>
				<string>OBJ</string>
			</array>
			<key>CFBundleTypeIconFile</key>
			<string>Slic3r.icns</string>
			<key>CFBundleTypeName</key>
			<string>STL</string>
			<key>CFBundleTypeRole</key>
			<string>Viewer</string>
            <key>LISsAppleDefaultForType</key>
            <true/>
            <key>LSHandlerRank</key>
            <string>Alternate</string>
		</dict>
		<dict>
			<key>CFBundleTypeExtensions</key>
			<array>
				<string>amf</string>
				<string>AMF</string>
			</array>
			<key>CFBundleTypeIconFile</key>
			<string>Slic3r.icns</string>
			<key>CFBundleTypeName</key>
			<string>STL</string>
			<key>CFBundleTypeRole</key>
			<string>Viewer</string>
            <key>LISsAppleDefaultForType</key>
            <true/>
            <key>LSHandlerRank</key>
            <string>Alternate</string>
		</dict>
	</array>
	<key>LSMinimumSystemVersion</key>
	<string>10.7</string>
    <key>NSPrincipalClass</key>
    <string>NSApplication</string>
    <key>NSHighResolutionCapable</key>
    <true/>
</dict>
</plist>
EOF

}
