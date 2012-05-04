#!/bin/sh
# used when developing smbta, easy way to kill everything
# running and restart, requires /etc/smbtad.conf


echo "killing torture and torturesrv..."
killall smbtatorture
killall smbtatorturesrv
sleep 1
echo "killing smbtad... and waiting 10 seconds"
killall smbtad
sleep 10
echo "restarting smb server"
rcsmb restart
sleep 1
echo "restaring smbtad"
smbtad -c /etc/smbtad.conf
echo "now run torture.sh  in /home/holger/SKRIPTE/torture.sh"

