from setuptools import setup, find_packages
import sys, os

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

blib = setup_py_dir + "/roboschool/cpp-household/bullet_local_install/lib"
if not os.path.exists(blib):
    print("Please follow instructions in README to build local (not global for your system) Bullet installation.")
    sys.exit(1)

from sys import platform
if platform=="darwin":
    bulletlibs  = "libBullet2FileLoader libBulletCollision libBullet3Collision libBulletDynamics libBullet3Common libBulletInverseDynamics".split()
    bulletlibs += "libBullet3Dynamics libBulletSoftBody libBullet3Geometry libLinearMath libBullet3OpenCL_clew libPhysicsClientC_API".split()
    for x in bulletlibs:
        os.system("install_name_tool -id @loader_path/cpp-household/bullet_local_install/lib/%s.2.87.dylib %s/%s.2.87.dylib" % (x,blib,x))
        for y in bulletlibs:
            os.system("install_name_tool -change @rpath/%s.2.87.dylib @loader_path/%s.2.87.dylib %s/%s.2.87.dylib" % (x,x,blib,y))

def recompile():
    global platform
    USE_PYTHON3 = ""
    if sys.version_info[0]==2:
        USE_PYTHON3 = "USE_PYTHON3=0"
    print("recompile for platform %s\n" % platform)
    if platform=="win32":
        sep = " && "
        magic = "echo %PATH% && "
        cmdprefix = "cmd /C \"%s" % magic
        cmdsuffix = "\""
        fileExt = "dll"
        make_prefix = "mingw32-"
        fileSuffix = "-cpython-36m"
    else:
        sep = " && "
        cmdprefix = ""
        cmdsuffix = ""
        fileExt = "so"
        make_prefix = ""
        fileSuffix = ""
    cmd = sep.join((
                    "cd %s/roboschool/cpp-household" % setup_py_dir,
                   "%smake clean" % make_prefix,
                   "%smake -j4 dirs %s ../cpp_household%s.%s" %(make_prefix, USE_PYTHON3,fileSuffix,fileExt)
                   ))
    cmd = "%s%s%s" % (cmdprefix,cmd,cmdsuffix)
    print(cmd)
    res = os.system(cmd)
    if res:
        print(dep)
        sys.exit(1)

class MyInstall(DistutilsInstall):
    def run(self):
        recompile()
        DistutilsInstall.run(self)

class MyEgg(EggInfo):
    def run(self):
        recompile()
        EggInfo.run(self)

if platform=="win32":
    fileExt = "dll"
    fileSuffix = "-cpython-36m"
else:
    fileExt = "so"
    fileSuffix = ""
need_files = ['cpp_household%s.%s' % (fileSuffix,fileExt)]
hh = setup_py_dir + "/roboschool"

for root, dirs, files in os.walk(hh):
    for fn in files:
        ext = os.path.splitext(fn)[1][1:]
        if ext and ext in 'png jpg urdf obj mtl dae off stl STL xml glsl so 87 dylib'.split():
            fn = root + "/" + fn
            need_files.append(fn[1+len(hh):])

print("found resource files: %i" % len(need_files))
#for n in need_files: print("-- %s" % n)

setup(
    name = 'roboschool',
    version = '1.0',
    description = 'OpenAI Household Simulator: mobile manipulation using Bullet',
    maintainer = 'Oleg Klimov',
    maintainer_email = 'omgtech@gmail.com',
    url = 'https://github.com/openai/roboschool',
    packages=[x for x in find_packages()],
    cmdclass = {
        'install': MyInstall,
        'egg_info': MyEgg
        },
    package_data = { '': need_files }
    )
