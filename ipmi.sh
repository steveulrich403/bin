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

case $command in
  sol) 
    ipmi_command="sol activate"
    ;;
  sold)
    ipmi_command="sol deactivate"
    ;;
  on)
    ipmi_command="power on"
    ;;
  off)
    ipmi_command="power off"
    ;;
  cycle)
    ipmi_command="power cycle"
    ;;
  boardrev)
    ipmi_command="raw 0x30 0x25"
    ;;
  *)
    error "unrecognized command '$command'.  Valid commands are:"
    commands
    ;;
esac

ipmitool -I lanplus -U $user -P $password -H $hostname $ipmi_command

exit 0
