if [[ $CC != clang* ]]; then
    echo "Stoat checks only supported for clang builds"
    exit
fi

apt-get install ruby

git clone https://github.com/fundamental/stoat && pushd stoat
mkdir build && cd build
cmake .. && make && make test
make install
popd

rm -r build/
export CC=stoat-compile
meson -C build/
stoat --recursive build/
