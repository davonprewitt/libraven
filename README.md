# Project Raven Core Library
VR Headset Controlled Drone
Design doc can be found here. 

This repository serves to develop pilot functionality for the Parrot Bebop 2 Drone as a standalone project to intergate as an Oculus Natuve C++ application. 

## Setup
```
git clone https://github.com/davonprewitt/libraven.git
cd raven

# Download ARDRoneSDK3
mkdir ARDroneSDK3 && cd ARDroneSDK3
repo init -u https://github.com/Parrot-Developers/arsdk_manifests.git -m release.xml
repo sync

# Compile ARDroneSDK3 binaries
/build.sh -p arsdk-native -t build-sdk -j
```

## Compile 
```
# For MacOS users
gcc -arch x86_64 main.c -o raven_exec -I/<PATH-TO-REPO>/libraven/ARDroneSDK3/out/arsdk-native/staging/usr/include /<PATH-TO-REPO>/libraven/ARDroneSDK3/out/arsdk-native/staging/usr/lib/*.dylib
```
## Running
```
./raven_exec
```
