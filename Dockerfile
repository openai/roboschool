FROM ubuntu:16.04
# get dependencies
RUN apt-get -y update && apt-get -y install python3-dev python3-pip curl \
               apt-utils libgl1-mesa-dev qtbase5-dev \
               libqt5opengl5-dev libassimp-dev patchelf cmake pkg-config git
# set python and pip to point to python3 versions
RUN bash -c "ln -s $(which python3) /usr/bin/python && ln -s $(which pip3) /usr/bin/pip"
COPY . /code
RUN  cd /code && \
     bash -c ". ./exports.sh && ./install_boost.sh && ./install_bullet.sh && ./roboschool_compile_and_graft.sh"

# install pip package and test
RUN pip install -e /code
RUN python -c "import gym; gym.make('roboschool:RoboschoolAnt-v1').reset()"
