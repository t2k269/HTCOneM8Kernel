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

#TOOLCHAIN=
TOOLCHAIN=linaro

export TOP=`pwd` 
if [ "$TOOLCHAIN" = "linaro" ]; then
	export PATH=$TOP/linaro/gcc/linux-x86/arm/arm-cortex_a15-linux-gnueabihf-linaro_4.9.3-2014.11/bin:~/bin:$PATH
else
	export PATH=$TOP/prebuilts/gcc/linux-x86/arm/arm-eabi-4.6/bin:~/bin:$PATH
fi
export ARCH=arm
export SUBARCH=arm
export CROSS_COMPILE=arm-eabi-
export ARCH=arm
export SUBARCH=arm
export TOOLCHAIN

export PS1="HTCOneM8Kernel($TOP):\w$"

