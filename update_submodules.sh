#!/bin/sh

ESCAPED_PWD=`echo $PWD | sed -e 's/\\//\\\\\\//g'`
cat .gitmodules | sed -e "s/https\:\/\/github.com\/t2k269\/HTCOneM8Kernel\.git/file:\/\/$ESCAPED_PWD/g" > new.gitmodules
mv new.gitmodules .gitmodules

git submodule init source
git checkout -- .gitmodules

git branch | grep m8cm12.0
if [ "$?" "!=" "0" ]; then
	git branch m8cm12.0 remotes/origin/m8cm12.0
fi
git submodule update
