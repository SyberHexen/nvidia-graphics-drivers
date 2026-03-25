#/bin/bash

dpkg-source --before-build ./
debuild -b
dpkg-buildpackage -b --host-arch=i386 -d
cd ./nvidia-settings
make
while true; do
    read -p "Do you wish to install nvidia-settings? " yn
    case $yn in
        [Yy]* ) sudo make install; break;;
        [Nn]* ) exit;;
        * ) echo "Please answer yes or no.";;
    esac
done
