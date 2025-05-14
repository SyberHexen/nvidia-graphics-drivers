#/bin/bash

dpkg-source --before-build ./
debuild -b
dpkg-buildpackage -b --host-arch=i386 -d
