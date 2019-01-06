#!/usr/bin/env bash
# Cribbed from https://github.com/darealshinji/AppImageKit-checkrt/blob/master/AppRun.sh

# some magic to find out the real location of this script dealing with symlinks
DIR=`readlink "$0"` || DIR="$0";
DIR=`dirname "$DIR"`;
cd "$DIR"
DIR=`pwd`
cd - > /dev/null

cxxpre=""
gccpre=""
execpre=""
libc6arch="libc6,x86-64"
if [ -n "$APPIMAGE" ] && [ "$(file -b "$APPIMAGE" | cut -d, -f2)" != " x86-64" ]; then
  libc6arch="libc6"
fi

cd "${DIR}/usr"

if [ -e "./optional/libstdc++/libstdc++.so.6" ]; then
  lib="$(PATH="/sbin:$PATH" ldconfig -p | grep "libstdc++\.so\.6 ($libc6arch)" | awk 'NR==1{print $NF}')"
  sym_sys=$(tr '\0' '\n' < "$lib" | grep -e '^GLIBCXX_3\.4' | tail -n1)
  sym_app=$(tr '\0' '\n' < "./optional/libstdc++/libstdc++.so.6" | grep -e '^GLIBCXX_3\.4' | tail -n1)
  if [ "$(printf "${sym_sys}\n${sym_app}"| sort -V | tail -1)" != "$sym_sys" ]; then
    cxxpath="./optional/libstdc++:"
  fi
fi

if [ -e "./optional/libgcc/libgcc_s.so.1" ]; then
  lib="$(PATH="/sbin:$PATH" ldconfig -p | grep "libgcc_s\.so\.1 ($libc6arch)" | awk 'NR==1{print $NF}')"
  sym_sys=$(tr '\0' '\n' < "$lib" | grep -e '^GCC_[0-9]\\.[0-9]' | tail -n1)
  sym_app=$(tr '\0' '\n' < "./optional/libgcc/libgcc_s.so.1" | grep -e '^GCC_[0-9]\\.[0-9]' | tail -n1)
  if [ "$(printf "${sym_sys}\n${sym_app}"| sort -V | tail -1)" != "$sym_sys" ]; then
    gccpath="./optional/libgcc:"
  fi
fi

# Don't load swrast_dir for now, it is breaking new systems and this mechanism doesn't 
# work for detecting whether or not it is necessary.
# if [ -e "./optional/swrast_dri/swrast_dri.so" ]; then
#   lib="$(PATH="/sbin:$PATH" ldconfig -p | grep "swrast_dri\.so ($libc6arch)" | awk 'NR==1{print $NF}')"
#   if [ "$lib" == "" ]; then
#       swrastpath=""
#   else
#       sym_sys=$(tr '\0' '\n' < "$lib" | grep -e '^GCC_[0-9]\\.[0-9]' | tail -n1)
#       sym_app=$(tr '\0' '\n' < "./optional/swrast_dri/swrast_dri.so" | grep -e '^GCC_[0-9]\\.[0-9]' | tail -n1)
#       if [ "$(printf "${sym_sys}\n${sym_app}"| sort -V | tail -1)" != "$sym_sys" ]; then
#         swrastpath="./optional/swrast_dri:"
#       fi
#   fi
# fi

if [ -n "$cxxpath" ] || [ -n "$gccpath" ] || [ -n "$swrastpath" ]; then
  if [ -e "./optional/exec.so" ]; then
    execpre=""
    export LD_PRELOAD="./optional/exec.so:${LD_PRELOAD}"
  fi
  export LD_LIBRARY_PATH="${cxxpath}${gccpath}${swrastpath}${LD_LIBRARY_PATH}"
fi

# disable parameter expansion to forward all arguments unprocessed to the VM
set -f
# run the VM and pass along all arguments as is
LD_LIBRARY_PATH="$DIR/usr/lib:${LD_LIBRARY_PATH}" "${DIR}/usr/bin/perl-local" -I"${DIR}/usr/lib/local-lib/lib/perl5" "${DIR}/usr/bin/slic3r.pl" --gui "$@"
exit $?
