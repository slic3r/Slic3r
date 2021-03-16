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
    if ($env:APPVEYOR_PULL_REQUEST_NUMBER -Or $env:APPVEYOR_REPO_BRANCH -ne "master" ) {
        if ($env:APPVEYOR_PULL_REQUEST_NUMBER) {
            winscp.com /privatekey=$KEY /command "open sftp://$UUSER@dl.slic3r.org/$DIR/branches -hostkey=*" "rm *PR${env:APPVEYOR_PULL_REQUEST_NUMBER}*" "exit"
        }
        if ($env:APPVEYOR_REPO_BRANCH -ne "master" ) {
            winscp.com /privatekey=$KEY /command "open sftp://$UUSER@dl.slic3r.org/$DIR/branches -hostkey=*" "rm *${env:APPVEYOR_REPO_BRANCH}*" "exit"
        }
        winscp.com /privatekey=$KEY /command "open sftp://$UUSER@dl.slic3r.org/$DIR/branches -hostkey=*" "put $UPLOAD ./$FILE" "exit"
    } else {
        winscp.com /privatekey=$KEY /command "open sftp://$UUSER@dl.slic3r.org/$DIR -hostkey=*" "put $UPLOAD ./$FILE" "exit"
    }
}
