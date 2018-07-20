#compile and install hoard

make clean
make linux-gcc-x86-64
sudo cp libhoard.so /usr/lib/
#sudo cp libhoard.so /usr/lib64/
sudo cp heaplayers/wrappers/gnuwrapper.h  /usr/include/gnuwrapper.h
