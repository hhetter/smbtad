#!/bin/sh

# This simple script is used when developing SMBTA
# it requires:
# two shares (namely SHARE1, SHARE2) with SMBTA enabled on a unix domain socket
# (see smb.conf)
# the script runs smbtatorture on these two shares, as user holger with
# password "linux"



smbtatorturesrv -p 3493 &
sleep 5

smbtatorture -P 3493 -t 20 -H localhost -v -u holger -p linux -1 smb://localhost/SHARE1/ -2 smb://localhost/SHARE2/ &

















