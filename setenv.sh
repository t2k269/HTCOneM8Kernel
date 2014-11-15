#!/bin/bash

function kbuild() {
	pushd $TOP/kitchen
	./build.sh
	popd
	ls $TOP/kitchen/*.zip
}

function kmake() {
	pushd $TOP/source
	make -j1
	popd
}

function kflash() {
	fastboot flash boot $TOP/kitchen/package/boot.img
}

export TOP=`pwd` 
if [ "$1" = "linaro" ]; then
	export PATH=$TOP/linaro/gcc/linux-x86/arm/arm-cortex_a15-linux-gnueabihf-linaro_4.9.2-2014.09/bin:~/bin:$PATH
else
	export PATH=$TOP/prebuilts/gcc/linux-x86/arm/arm-eabi-4.6/bin:~/bin:$PATH
fi
export ARCH=arm
export SUBARCH=arm
export CROSS_COMPILE=arm-eabi-
export ARCH=arm
export SUBARCH=arm

export PS1="HTCOneM8Kernel($TOP):\w$"

