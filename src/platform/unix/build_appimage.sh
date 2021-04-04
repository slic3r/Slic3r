#!/bin/sh
APPIMAGETOOLURL="https://github.com/AppImage/AppImageKit/releases/latest/download/appimagetool-x86_64.AppImage"


APP_IMAGE=$1

wget ${APPIMAGETOOLURL} -O ../appimagetool.AppImage
chmod +x ../appimagetool.AppImage

sed -i -e 's#/usr#././#g' bin/Slic3r
mv Slic3r AppRun
chmod +x AppRun

cp resources/icons/Slic3r_192px.png Slic3r.png
cat <<EOF > Slic3r.desktop
[Desktop Entry]
Name=Slic3r
Exec=AppRun %F
Icon=Slic3r
Type=Application
Categories=Utility;
MimeType=model/stl;application/vnd.ms-3mfdocument;application/prs.wavefront-obj;application/x-amf;
EOF


../appimagetool.AppImage .
mv Slic3r-x86_64.AppImage ${APP_IMAGE}
chmod +x ${APP_IMAGE}
