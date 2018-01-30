#!/bin/bash

function commands()
{
    echo "Command List:"
    echo "sol           Activate Serial Over LAN"
    echo "soldl         deactivate Serial Over LAN"
    echo "on            power on"
    echo "off           power off"
    echo "cycle         power cycle"
    echo "boardrev      query board revision"
    echo "cpldrev       query CPLD revision"
}

function usage()
{
    echo $1
    echo "usage: $0 -H <host> -T <rep/sdp> -C cmd"
    commands
    exit 1
}

batch=0
verbose=0
hostname=""
hosttype=""
command=""

while getopts ":vb:d:H:T:C:" opt; do
  case ${opt} in
    v ) verbose=1
      ;;
    b ) batch=1
      ;;
    H ) hostname=$OPTARG
      ;;
    T ) hosttype=$OPTARG
      ;;
    C ) command=$OPTARG
      ;;
    \? ) usage
      ;;
  esac
done

if [ "$hostname" = "" ] ; then
    usage "no host name specified"
fi

if [ "$hosttype" = "" ] ; then
    usage "no host type specified"
fi

if [ $hosttype = rep ] ; then
    user=admin
    password=admin
elif [ $hosttype = sdp ] ; then
    user=admin
    password=Password1
else
    usage "uncognized host type - host type must be \'rep\' or \'sdp\'"
fi 

if [ "$command" = "" ] ; then
    usage "no command specified"
fi

declare -a ipmi_command

case $command in
  sol) 
    ipmi_command[0]="sol activate"
    ;;
  sold)
    ipmi_command[0][1]="sol deactivate"
    ;;
  on)
    ipmi_command[0]="power on"
    ;;
  off)
    ipmi_command[0]="power off"
    ;;
  cycle)
    ipmi_command[0]="power cycle"
    ;;
  boardrev)
    ipmi_command[0]="raw 0x30 0x25"
    ;;
  cpldrev)
    ipmi_command[0]="i2c bus=private chan=6 0x1e 1 1"
    ipmi_command[1]="i2c bus=private chan=6 0x1e 1 0"
    ;;
  *)
    error "unrecognized command '$command'.  Valid commands are:"
    commands
    ;;
esac




for i in `seq 0 $(expr ${#ipmi_command[@]} - 1)` ; do
    echo $i
    echo ${ipmi_command[$i]}
    ipmitool -I lanplus -U $user -P $password -H $hostname ${ipmi_command[$i]}
done

exit 0
