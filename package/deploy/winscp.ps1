# Prerequisites
# Environment Variables:
#   UPLOAD_USER - user to upload to sftp server
# KEY is assumed to be path to a ssh key for UPLOAD_USER

param (
    [parameter(Mandatory=$true, ParameterSetName="dir")] [string]$DIR,
    [parameter(Mandatory=$true, ParameterSetName="key")] [string]$KEY,
    [parameter(Mandatory=$true, ParameterSetName="file")] [string]$FILES,

)
UUSER=$env:UPLOAD_USER
winscp.exe scp://$UUSER@dl.slic3r.org:$DIR /hostkey=* /privatekey=$KEY /upload $FILES
