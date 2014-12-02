#!/bin/bash

cd $TOP/source

git fetch cm11_htc_msm8974

LAST_HASH=`git rev-parse remotes/cm11_htc_msm8974/cm-12.0`

NEW_COMMIT=0
if [[ "`git branch --contains $LAST_HASH m8cm12.0`" == "" ]]; then
	NEW_COMMIT=1
fi

if [[ NEW_COMMIT -eq 0 ]]; then
	exit 0;
fi

# New commit found
git merge remotes/cm11_htc_msm8974/cm-12.0
if [[ $? -ne 0 ]]; then
	echo "Conflict while merging from CyanogenMod kernel https://github.com/CyanogenMod/android_kernel_htc_msm8974/commit/$LAST_HASH"
	exit 1;
fi

make cm_m8_defconfig
make -j1
if [[ $? -ne 0 ]]; then
	echo "Build failed!"
	exit 1;
fi

git push origin

cd $TOP/kitchen
./build.sh

ZIPFILE=`ls kernel-*.zip`
./dropbox_uploader.sh upload $ZIPFILE Kernel/
echo "New kernel $ZIPFILE are available in dropbox"

