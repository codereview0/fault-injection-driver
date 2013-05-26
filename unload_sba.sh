#!/bin/sh
module="SBA"
device="SBA"

# invoke rmmod with all arguments we got
/sbin/rmmod $module $* || exit 1

# Remove stale nodes

rm -f /dev/${device} /dev/${device}0 /dev/${device}1
