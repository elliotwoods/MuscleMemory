# Introduction

We use:

* The ES32 microcontroller hardware
* The ESP-IDF toolchain
* The MicroPython framework / environment

And generally we are building in a Windows environment. The following document is a guide on how to get Muscle Memory firmware building to a `firmware.bin` file which can be uploaded to the ESP32.

Note that this build process requires a significant amount of disk space (e.g. a couple of GBs) and installing WSL which enables Hyper-V in Windows. I've had some problems with other applications complaining about Hyper-V being enabled before, so be warned.

Useful instructions are also available at https://github.com/micropython/micropython/tree/master/ports/esp32

Into the future, it would be great to have a workflow using some kind of build server, or to be able to build  MicroPython with  PlatformIO.

# WSL

The best way we've found so far for building MicroPython with ESP-IDF under windows is using the WSL (Windows Subsystem for Linux). Currently we are using WSL2.

To install WSL:

1. Open the Microsoft Store (Press Start. Type "Store")
2. Search for Ubuntu and install
3. Follow the instructions (remember the password which you set here - you'll need it)

Now let's set up a few things in the WSL environment:

```
sudo apt-get update
sudo apt-get install python make
```

Note that your C drive is available at `/mnt/C`

Choose a folder where you'll install everything. I generally choose `/mnt/C/dev`

# Windows Terminal (Optional)

This is optional. It is also available in the Microsoft Store. You might also want to add a section to the settings.json file for Git Bash:

```json
           {
                "guid": "{939473b0-a9b2-4325-b669-6b4ac5a0c0d4}",
                "hidden": false,
                "name": "Git Bash",
                "commandline": "%PROGRAMFILES%/git/usr/bin/bash.exe -i -l",
                "icon": "%PROGRAMFILES%/git/mingw64/share/git/git-for-windows.ico",
                "startingDirectory": "C:\\dev"
            }
```

Note that the paths may be different on your machine

# ESP-IDF and MicroPython

In WSL, `cd` into the folder where you'll be installing things, e.g. `cd /mnt/c/dev`.

(Note : some operations are quicker if called from the Git Bash rather than WSL, e.g. pulling everything from github.)

We currently use ESP-IDF v3.3 (since Platform IO doesn't seem to support v4.0 properly yet. Or at least we can't get that working. And we do a lot of testing in PlatformIO before integrating into MicroPython).

```
# Checkout most recent micropython
git clone https://github.com/micropython/micropython.git --recursive --depth 1

# Checkout a specific version of ESP-IDF (v3.3)
git clone https://github.com/espressif/esp-idf.git
cd esp-idf
git checkout 9e70825d1e1cbf7988cf36981774300066580ea7
git submodule update --init --recursive

```

Now we're going to install some python dependencies which ESP-IDF needs to operate

```
curl https://bootstrap.pypa.io/get-pip.py -o get-pip.py
python ./get-pip.py
pip install --updgrade pip
pip install -r esp-idf/requirements.txt
pip install esptool
```


And install the ESP-IDF

```
esp-idf/install.sh
```

And then in the future to activate it

```
source esp-idf/export.sh
```

This needs to happen for each bash session where you want to build MicroPython.

The micropython build process needs python3, so let's install that

```
sudo apt-get install python3
```

We need to build mpy-cross:

```
cd micropython/mpy-cross
make -j
cd ../..
```

## Berkley DB issue

There's one final issue with building micropython under WSL, and it comes from the fact that part of the build process expects a 
Now let's test building micropython esp32 port with default modules...

```
cd micropython/lib/berkeley-db-1.xx
git remote add elliot https://github.com/elliotwoods/berkeley-db-1.xx.git
git pull elliot wsl-fix
cd ../../..
```

# Building micropython

## Building with no user modules

This builds with the default micropython functionality

```
# you need to do this once per session
source esp-idf/export.sh

cd micropython/ports/esp32
make -j
```