#!/bin/bash

cd $TOP/source

git fetch cm11_htc_msm8974 > /dev/null 2>&1

LAST_HASH=`git rev-parse remotes/cm11_htc_msm8974/cm-12.0`

NEW_COMMIT=0
if [[ "`git rev-parse m8cm12.0`" != "$LAST_HASH" ]]; then
	NEW_COMMIT=1
fi

if [[ NEW_COMMIT -eq 0 ]]; then
	exit 0;
fi

echo New commit found

git checkout m8cm12.0
git reset --hard HEAD
git pull
if [[ $? -ne 0 ]]; then
	git reset --hard HEAD
	echo "Conflict while merging from CyanogenMod kernel https://github.com/CyanogenMod/android_kernel_htc_msm8974/commit/$LAST_HASH"
	exit 1
fi

HEAD_HASH=`git rev-parse m8cm12.0`
git checkout m8cm12.0_sp2w
git reset --hard $HEAD_HASH 
git cherry-pick 3d5f2fd3e27dacb5cb17ee26489ca9106a4f18ed 
if [[ $? -ne 0 ]]; then
	git cherry-pick --abort
	echo "Unable to apply patch SP2W"
	exit 1
fi

make cm_m8_defconfig
make -j1
if [[ $? -ne 0 ]]; then
	echo "Build failed!"
	exit 1;
fi

cd $TOP/kitchen
./build.sh

cd $TOP/source

git push origin --force

git checkout m8cm12.0
git push origin

cd $TOP/kitchen
ZIPFILE=`ls kernel-*.zip`
./dropbox_uploader.sh upload $ZIPFILE Kernel/
echo "New kernel $ZIPFILE are available in dropbox"

