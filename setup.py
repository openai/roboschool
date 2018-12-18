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
# blib = setup_py_dir + "/roboschool/cpp-household/bullet_local_install/lib"
# if not os.path.exists(blib):
#     print("Please follow instructions in README to build local (not global for your system) Bullet installation.")
#     sys.exit(1)

# from sys import platform
# if platform=="darwin":
#     bulletlibs  = "libBullet2FileLoader libBulletCollision libBullet3Collision libBulletDynamics libBullet3Common libBulletInverseDynamics".split()
#     bulletlibs += "libBullet3Dynamics libBulletSoftBody libBullet3Geometry libLinearMath libBullet3OpenCL_clew libPhysicsClientC_API".split()
#     for x in bulletlibs:
#         os.system("install_name_tool -id @loader_path/cpp-household/bullet_local_install/lib/%s.2.87.dylib %s/%s.2.87.dylib" % (x,blib,x))
#         for y in bulletlibs:
#             os.system("install_name_tool -change @rpath/%s.2.87.dylib @loader_path/%s.2.87.dylib %s/%s.2.87.dylib" % (x,x,blib,y))

# with open(os.path.join(os.path.dirname(__file__), 'atari_py/package_data.txt')) as f:
#    package_data = [line.rstrip() for line in f.readlines()]

class CMakeExtension(Extension):
    def __init__(self, name, sourcedir=''):
        Extension.__init__(self, name, sources=[])
        self.sourcedir = os.path.abspath(sourcedir) 
 

class Build(build_ext):
   def run(self):
       pass

def recompile():
    import subprocess
    USE_PYTHON3 = ""
    if sys.version_info[0]==2:
        USE_PYTHON3 = "USE_PYTHON3=0"
    
    subprocess.check_call(['bash', 'roboschool_compile_and_graft.sh'])
    
# recompile()
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
    version = '1.0.34',
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
