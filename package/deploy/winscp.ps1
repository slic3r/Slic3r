# Prerequisites
# Environment Variables:
#   UPLOAD_USER - user to upload to sftp server
# KEY is assumed to be path to a ssh key for UPLOAD_USER

Param(
    [string]$DIR,
    [string]$KEY,
    [string]$FILE
)
Set-Variable -Name "UUSER" -Value "$env:UPLOAD_USER"
winscp.exe /command sftp://$UUSER@dl.slic3r.org:$DIR/ /hostkey=* /privatekey=$KEY /upload "put $FILE"
