# # rm -r .build # debug
# # rm -r .install

# # Add the standalone toolchain to the search path.
# export PATH=$PATH:/mnt/d/tools/android/android_toolchain_linux/bin

# # Tell configure what tools to use.
# target_host=aarch64-linux-android
# export AR=$target_host-ar
# export AS=$target_host-clang
# export CC=$target_host-clang
# export CXX=$target_host-clang++
# export LD=$target_host-ld
# export STRIP=$target_host-strip

# # Tell configure what flags Android requires.
# export CFLAGS="-fPIE -fPIC"
# export LDFLAGS="-pie"

# # Invoke CMake to generate Makefiles (default on Linux)
# # go to `CMakeLists.txt` to see what is actually scripted to be done
# cmake . -B.build -DCMAKE_INSTALL_PREFIX=$PWD/.install

# # Invoke CMake's build mode, which is really just calling `make`
# cmake --build .build --target install --config Release

# # Test exe on android
# adb start-server                            # initialize adb
# adb root                                    # gain device root access
# adb push .install/bin/test_main data/local  # push exe to device
# adb shell ./data/local/test_main            # run exe

# opencv_source_dir=/home/yunghsiu_chen/flame_dependency/src/opencv-3.3.1

android_ndk_dir=/home/andyyc_chen/android-ndk-r17c
android_api_level=26
android_abi=arm64-v8a

# opencv_install_dir=/home/yunghsiu_chen/flame_dependency/toAndroid/opencv3.3.1_android${android_api_level}_${android_abi}
opencv_install_dir=/home/andyyc_chen/code/android_thirdparty/OpenCV-android-sdk/

cmake . -Bbuild \
-DCMAKE_TOOLCHAIN_FILE=$android_ndk_dir/build/cmake/android.toolchain.cmake \
-DANDROID_NDK=$android_ndk_dir \
-DANDROID_NATIVE_API_LEVEL=android-$android_api_level \
-DANDROID_STL=c++_static \
-DANDROID_ABI=$android_abi \
-DCMAKE_INSTALL_PREFIX=install \
-DOpenCV_DIR=$opencv_install_dir/sdk/native/jni/abi-arm64-v8a

cmake --build build --config Release
