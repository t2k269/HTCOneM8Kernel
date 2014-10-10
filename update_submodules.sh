#!/bin/sh

ESCAPED_PWD=`echo $PWD | sed -e 's/\\//\\\\\\//g'`
cat .gitmodules | sed -e "s/https\:\/\/github.com\/t2k269\/HTCOneM8Kernel\.git/file:\/\/$ESCAPED_PWD/g" > new.gitmodules
mv new.gitmodules .gitmodules

git submodule init source
git checkout -- .gitmodules
