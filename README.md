
NEWS
====

**2017 July 17, Version 1.1**

* All envs version bumped to “-v1", due to stronger stuck joint punishment, that improves odds of getting a good policy.
* Flagrun-v1 is much more likely to develop a symmetric gait,
* FlagrunHarder-v1 has new "repeat-underlearned" learning schedule, that allows it to be trained to stand up, walk and turn without falling. 
* Atlas robot model, modified (empty links removed, overly powerful feet weakaned).
* All -v1 envs are shipped with better zoo policies, compared to May versions.
* Keyboard-controlled humanoid included.


Roboschool
==========

Release blog post is here:

https://blog.openai.com/roboschool/


Roboschool is a long-term project to create simulations useful for research. The roadmap is as follows:

1. Replicate Gym MuJoCo environments.
2. Take a step away from trajectory-centric fragile MuJoCo tasks.
3. Explore multiplayer games.
4. Create tasks with camera RGB image and joints in a tuple.
5. Teach robots to follow commands, including verbal commands.


Some wiki pages:

[Contributing New Environments](https://github.com/openai/roboschool/wiki/Contributing-New-Environments)

[Help Wanted](https://github.com/openai/roboschool/wiki/Help-Wanted)



Environments List
=================

The list of Roboschool environments is as follows:

- RoboschoolInvertedPendulum-v0
- RoboschoolInvertedPendulumSwingup-v0
- RoboschoolInvertedDoublePendulum-v0
- RoboschoolReacher-v0
- RoboschoolHopper-v0
- RoboschoolWalker2d-v0
- RoboschoolHalfCheetah-v0
- RoboschoolAnt-v0
- RoboschoolHumanoid-v0
- RoboschoolHumanoidFlagrun-v0
- RoboschoolHumanoidFlagrunHarder-v0
- RoboschoolPong-v0

To obtain this list: `import roboschool, gym; print("\n".join(['- ' + spec.id for spec in gym.envs.registry.all() if spec.id.startswith('Roboschool')]))`.



Installation
============

First, define a `ROBOSCHOOL_PATH` variable in the current shell. It will be used in this README but not anywhere in the Roboschool code.

```bash
ROBOSCHOOL_PATH=/path/to/roboschool
```

If you have both Python2 and Python3 on your system, use `python3` and `pip3` commands.

The dependencies are gym, Qt5, assimp, tinyxml, and bullet (from a branch). For the non-bullet deps, there are several options, depending on what platform and package manager you are using.

- Ubuntu:

    ```bash
    apt install cmake ffmpeg pkg-config qtbase5-dev libqt5opengl5-dev libassimp-dev libpython3.5-dev libboost-python-dev libtinyxml-dev
    ```

    Users report in issue #15 that `sudo pip3 install pyopengl` can make OpenGL errors go away, because it arranges OpenGL libraries in
    an Ubuntu system in the right way.



- Linuxbrew

    ```bash
    brew install boost-python --without-python --with-python3 --build-from-source
    export C_INCLUDE_PATH=/home/user/.linuxbrew/include:/home/user/.linuxbrew/include/python3.6m
    export CPLUS_INCLUDE_PATH=/home/user/.linuxbrew/include:/home/user/.linuxbrew/include/python3.6m
    export LIBRARY_PATH=/home/user/.linuxbrew/lib
    export PKG_CONFIG_PATH=/home/user/.linuxbrew/lib/pkgconfig:/usr/lib/pkgconfig:/usr/lib/x86_64-linux-gnu/pkgconfig
    ```

    (still use Qt from Ubuntu, because it's known to work)

- Mac, homebrew python:

    ```bash
    # Will not work on Mavericks: unsupported by homebrew, some libraries won't compile, upgrade first
    brew install python3
    brew install cmake tinyxml assimp ffmpeg qt
    brew reinstall boost-python --without-python --with-python3 --build-from-source
    export PATH=/usr/local/bin:/usr/local/opt/qt5/bin:$PATH
    export PKG_CONFIG_PATH=/usr/local/opt/qt5/lib/pkgconfig
    ```

- Mac, Anaconda with Python 3

    ```bash
    brew install cmake tinyxml assimp ffmpeg
    brew reinstall boost-python --without-python --with-python3 --build-from-source
    conda install qt
    export PKG_CONFIG_PATH=$(dirname $(dirname $(which python)))/lib/pkgconfig
    ```


Compile and install bullet as follows. Note that `make install` will merely copy files into the roboschool directory.

```bash
git clone https://github.com/olegklimov/bullet3 -b roboschool_self_collision
mkdir bullet3/build
cd    bullet3/build
cmake -DBUILD_SHARED_LIBS=ON -DUSE_DOUBLE_PRECISION=1 -DCMAKE_INSTALL_PREFIX:PATH=$ROBOSCHOOL_PATH/roboschool/cpp-household/bullet_local_install -DBUILD_CPU_DEMOS=OFF -DBUILD_BULLET2_DEMOS=OFF -DBUILD_EXTRAS=OFF  -DBUILD_UNIT_TESTS=OFF -DBUILD_CLSOCKET=OFF -DBUILD_ENET=OFF -DBUILD_OPENGL3_DEMOS=OFF ..
make -j4
make install
cd ../..
```

Finally, install project itself:

```bash
pip3 install -e $ROBOSCHOOL_PATH
```

Now, check to see if it worked by running a pretrained agent from the agent zoo.


Agent Zoo
=========

We have provided a number of pre-trained agents in the `agent_zoo` directory.

To see a humanoid run towards a random varying target:

```bash
python $ROBOSCHOOL_PATH/agent_zoo/RoboschoolHumanoidFlagrun_v0_2017may.py
```

To see three agents in a race:

```bash
python $ROBOSCHOOL_PATH/agent_zoo/demo_race2.py
```
