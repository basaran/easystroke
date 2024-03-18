Compilation notes from the previous patches:

Debian/Ubuntu
```
sudo apt-get install g++ libboost-serialization-dev libgtkmm-3.0-dev libxtst-dev libdbus-glib-1-dev intltool xserver-xorg-dev
```
or
```
sudo apt-get build-dep easystroke
```
NOTE: Easystroke 5.x and below need libgtkmm-2.4-dev to build, not 3.0.

Fedora
```
yum install gcc-c++ gtkmm30-devel dbus-glib-devel boost-devel libXtst-devel intltool
```
NOTE: You might have to install 'xorg-x11-server-devel' for easystroke to compile probably (I had to).

OpenSUSE
```
zypper in make gcc gcc-c++ gtkmm2-devel glib2-devel dbus-1-glib-devel boost-devel intltool
```
NOTE: You might have to install 'xorg-x11-server-sdk' to successfully build on 12.X.

Pardus
```
pisi it make gcc  gtkmm boost-devel intltool
```
For Pardus, you also have to add -lXi to the LIBS row in the Makefile in order to get the app to compile.

Manjaro (Arch)
```
pacman -S boost-libs xorg-server-devel
```
