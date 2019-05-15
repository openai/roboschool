from setuptools import setup, find_packages
from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext
import subprocess
import sys, os
import re

dep = """
C++ dependencies for this project are:

bullet
tinyxml
boost_python
assimp
Qt5

If you see compilation error FIRST THING TO CHECK if pkg-config call was successful.
Install dependencies that pkg-config cannot find.
"""

from setuptools.command.install import install as DistutilsInstall
from setuptools.command.egg_info import egg_info as EggInfo

setup_py_dir = os.path.dirname(os.path.realpath(__file__))

class CMakeExtension(Extension):
    def __init__(self, name, sourcedir=''):
        Extension.__init__(self, name, sources=[])
        self.sourcedir = os.path.abspath(sourcedir) 
 

class Build(build_ext):
   def run(self):
       pass

need_files = ['cpp_household.so']
hh = setup_py_dir + "/roboschool"
need_files_ext = 'png jpg urdf obj mtl dae off stl STL xml glsl dylib'.split()
need_files_re = [re.compile(r'.+\.'+p) for p in need_files_ext]
need_files_re.append(re.compile(r'.+\.so(.\d+)*'))
need_files_re.append(re.compile(r'.+/\.libs/.+'))
need_files_re.append(re.compile(r'.+/\.qt_plugins/.+'))

for root, dirs, files in os.walk(hh):
    for fn in files:
        fn = root + '/' + fn
        if any([p.match(fn) for p in need_files_re]):
            need_files.append(fn[1+len(hh):])

print("found resource files: %i" % len(need_files))
for n in need_files: print("-- %s" % n)

setup(
    name = 'roboschool',
    version = '1.0.48',
    description = 'OpenAI Household Simulator: mobile manipulation using Bullet',
    maintainer = 'Oleg Klimov',
    maintainer_email = 'omgtech@gmail.com',
    url = 'https://github.com/openai/roboschool',
    packages=[x for x in find_packages()],
    ext_modules=[CMakeExtension('roboschool')],
    cmdclass={'build_ext': Build},
    install_requires=['gym'],
    package_data = { '': need_files }
)
