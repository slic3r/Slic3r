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
Set-Variable -Name "UPLOAD" -Value "$($FILE | Resolve-Path)"
if (Test-Path $KEY) {
    winscp.com /privatekey=$KEY /command "open sftp://$UUSER@dl.slic3r.org/$DIR -hostkey=*" "put $UPLOAD ./$FILE" "exit"
}
