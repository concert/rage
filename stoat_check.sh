set -e

if [[ $CC != clang* ]]; then
    echo "Stoat checks only supported for clang builds"
    exit
fi

#ln -s /usr/lib/libstdc++.so.6 /usr/lib/libstdc++.so

git clone https://github.com/fundamental/stoat && pushd stoat
mkdir build && cd build
cmake .. && make && make test
make install
popd

export CC=stoat-compile
rm -r build/
mkdir build
meson build
ninja -C build
stoat --recursive build/
