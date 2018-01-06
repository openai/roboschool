UNAME := $(shell uname -s)
OBJDIRR=.build-release
OBJDIRD=.build-debug
PKG_P := $(PKG_CONFIG_PATH)
$(info PWD $(shell /bin/bash -c 'pwd'))
ifeq ($(UNAME),MSYS_NT-10.0)
	export PKG_CONFIG_PATH=$(shell /bin/bash -c 'pwd -W')\bullet_local_install\lib\pkgconfig\;$(PKG_P)
else
	export PKG_CONFIG_PATH=$(shell /bin/bash -c 'pwd')\bullet_local_install\lib\pkgconfig\;$(PKG_P)
endif

$(info PKG_CONFIG_PATH is )
$(info ${PKG_CONFIG_PATH})



WIN_LIBS=
ifeq ($(UNAME),MSYS_NT-10.0)
  PKG  =/mingw64/bin/pkg-config
  MOC  =moc
  LIBS =-L/mingw64/bin -L/mingw64/lib -lm -lopengl32 -lmsvcr100
  INC  =-I/mingw64/x86_64-w64-mingw32/include/ -I/mingw64/include/
  
  BOOST_MT=
  ifneq ($(USE_PYTHON3),0)
    HAVE_PYTHON3=$(shell which python3)
  endif
  ifneq ($(HAVE_PYTHON3),)
    PYTHON = $(shell python3 -c "import sys; print('%i.%i' % sys.version_info[:2])")
    ifneq (,$(findstring linuxbrew,$(HAVE_PYTHON3)))
      $(info Linux, python3, linuxbrew branch)
      BOOST_PYTHON3_POSTFIX = 3
    else ifneq (,$(findstring conda,$(HAVE_PYTHON3)))
      $(info Linux, python3, anaconda branch, will query Ubuntu python3 for boost_python)
      BOOST_PYTHON3_POSTFIX = $(shell /usr/bin/python3 -c "import sys; print('%i%i' % sys.version_info[:2])")-mt
    else
      $(info Linux, python3, Ubuntu branch)
      BOOST_PYTHON3_POSTFIX = 3-mt
    endif
  else
    $(info Linux, python 2 branch)
    PYTHON = $(shell python -c "import sys; print('%i.%i' % sys.version_info[:2])")
  endif
  RPATH=-Wl,-rpath,'$$ORIGIN/cpp-household/bullet_local_install/lib' -Wl,-rpath,'$$ORIGIN/cpp-household/bullet_local_install/bin'
endif

ifeq ($(UNAME),Linux)
  PKG  =pkg-config
  MOC  =moc -qt=5
  ifneq ($(CROSS_MINGW),1)
	 LIBS =-L/usr/lib64 -lm -lGL -lGLU
  endif
  INC  =-I/usr/include
  ifeq ($(CROSS_MINGW),1)
	INC=-I bullet_local_install/include
	INC+=-I bullet_local_install/include/bullet/
	INC+=-I ../../boost/mingw64/include
	INC+=-I ../../qt/mingw64/include
	INC+=-I ../../assimp/mingw64/include
	INC+=-I ../../python/mingw64/include/python3.6m
  endif

  BOOST_MT=
  ifneq ($(USE_PYTHON3),0)
    HAVE_PYTHON3=$(shell which python3)
  endif
  ifneq ($(HAVE_PYTHON3),)
    PYTHON = $(shell python3 -c "import sys; print('%i.%i' % sys.version_info[:2])")
    ifneq (,$(findstring linuxbrew,$(HAVE_PYTHON3)))
      $(info Linux, python3, linuxbrew branch)
      BOOST_PYTHON3_POSTFIX = 3
    else ifneq (,$(findstring conda,$(HAVE_PYTHON3)))
      $(info Linux, python3, anaconda branch, will query Ubuntu python3 for boost_python)
      BOOST_PYTHON3_POSTFIX = -py$(shell /usr/bin/python3 -c "import sys; print('%i%i' % sys.version_info[:2])")
    else
      $(info Linux, python3, Ubuntu branch)
      BOOST_PYTHON3_POSTFIX = -py$(shell python3 -c "import sys; print('%i%i' % sys.version_info[:2])")
    endif
  else
    $(info Linux, python 2 branch)
    PYTHON = $(shell python -c "import sys; print('%i.%i' % sys.version_info[:2])")
  endif
  ifneq ($(CROSS_MINGW),1)
	  RPATH=-Wl,-z,origin -Wl,--disable-new-dtags -Wl,-rpath,'$$ORIGIN/cpp-household/bullet_local_install/lib'
  endif
endif

ifeq ($(UNAME),Darwin)
  ifneq ("$(wildcard /usr/local/opt/qt5/bin/moc"), "")
    $(info Mac, assumed python3, homebrew branch)
    MOC =/usr/local/opt/qt5/bin/moc
  else
    $(info Mac, assumed python3, anaconda branch)
    MOC =moc
  endif
  PKG   =pkg-config
  LIBS  =-framework OpenGL
  INC   =-I/System/Library/Frameworks/OpenGL.framework/Headers
  BOOST_PYTHON3_POSTFIX = 3
  BOOST_MT=-mt
  PYTHON = $(shell python3 -c "import sys; print('%i.%i' % sys.version_info[:2])")
  RPATH =
endif

$(info Link against python $(PYTHON))
INC     += `$(PKG) --cflags bullet Qt5Core Qt5Widgets Qt5OpenGL assimp python-$(PYTHON) $(WIN_LIBS)`
ifeq ($(CROSS_MINGW),1)
	LIBS += ../../qt/mingw64/lib/libQt5Core.dll.a ../../qt/mingw64/lib/libQt5OpenGL.dll.a ../../qt/mingw64/lib/libQt5Widgets.dll.a ../../qt/mingw64/lib/libQt5Gui.dll.a
	LIBS += ../../precompiled/libpython36.a
	LIBS += ../../precompiled/libboost_python3-mgw72-mt-x64-1_66.dll.a
	LIBS += ../../assimp/mingw64/lib/libassimp.dll.a
	BOOST_PYTHON=
else
	LIBS    += -lstdc++ `$(PKG) --libs Qt5Core Qt5OpenGL Qt5Widgets assimp python-$(PYTHON) $(WIN_LIBS)`
	ifeq ($(PYTHON),2.7)
	    BOOST_PYTHON = -lboost_python
	else
	    BOOST_PYTHON = -lboost_python$(BOOST_PYTHON3_POSTFIX)
	endif
endif
#INC     += -Ibullet_local_install/include -Ibullet_local_install/include/bullet
ifeq ($(UNAME),MSYS_NT-10.0)
    INC     += -I/mingw64/usr/local/include/bullet
else
    INC     += -I/usr/local/include/bullet
endif
#LIBS    += $(RPATH) -Lbullet_local_install/lib -lLinearMath -lBullet3Common -lBulletCollision -lBulletDynamics -lBulletInverseDynamics -lPhysicsClientC_API
#LIBS    += $(RPATH) `$(PKG) --libs bullet`
LIBS +=$(shell ls bullet_local_install/lib/*.a)

CC=gcc
LINK=g++

ifeq ($(CROSS_MINGW),1)
	CC=x86_64-w64-mingw32-gcc
	LINK=x86_64-w64-mingw32-g++
endif

AR=ar r
FILEEXT=so
AR_OUT=
LINK_OUT = -o
MINUS_O  = -o
CFLAGS   = -std=c++11 -Wall -Wno-unused-variable -Wno-unused-function -Wno-deprecated-register -fPIC -DBT_USE_DOUBLE_PRECISION -g -O3 -march=native $(INC)
CFLAGSD  = -std=c++11 -Wall -Wno-unused-variable -Wno-unused-function -Wno-deprecated-register -fPIC -DBT_USE_DOUBLE_PRECISION -g -DDEBUG $(INC)
LINK_FLAGS_DLL =
ifeq ($(UNAME),MSYS_NT-10.0)
    FILEEXT=pyd
    CC=/mingw64/bin/gcc
    LINK=/mingw64/bin/g++
    CFLAGS  += -D M_PI=3.14159265358979323846 -Dmain=SDL_main -D_GLIBCXX_USE_CXX11_ABI=0
    CFLAGSD += -D M_PI=3.14159265358979323846 -Dmain=SDL_main -D_GLIBCXX_USE_CXX11_ABI=0
    LINK_FLAGS_DLL = -Wl,--dll
endif
ifeq ($(CROSS_MINGW),1)
    FILEEXT=pyd
    CFLAGS  += -D M_PI=3.14159265358979323846 -Dmain=SDL_main -D_GLIBCXX_USE_CXX11_ABI=0
    CFLAGSD += -D M_PI=3.14159265358979323846 -Dmain=SDL_main -D_GLIBCXX_USE_CXX11_ABI=0
endif


SHARED  = -shared
DEPENDS = -MMD -MF $@.dep

ifeq ($(CROSS_MINGW),1)
	EVERY_BIN=../cpp_household.$(FILEEXT)
else
	EVERY_BIN=../robot-test-tool ../robot-test-tool_d ../cpp_household.$(FILEEXT) ../cpp_household_d.$(FILEEXT)
endif

SIM = \
 physics-bullet.cpp \
 assets-mesh.cpp \
 random-world-tools.cpp \
 render-glwidget.cpp \
 render-hud.cpp \
 render-simple.cpp \
 render-simple-primitives.cpp

ifneq ("$(wildcard /usr/lib/x86_64-linux-gnu/libGLX_nvidia.so.0)", "")
$(info Hardware render (turn on shadows))
SIM     += render-ssao.cpp
CFLAGS  += -DUSE_SSAO
CFLAGSD += -DUSE_SSAO
else
$(info Slow hardware or software render (no shadows))
endif

TWND = \
 test-tool-qt4.cpp

PYTH = python-binding.cpp

SIM_R = $(patsubst %.cpp, $(OBJDIRR)/%.o, $(SIM))
SIM_D = $(patsubst %.cpp, $(OBJDIRD)/%.o, $(SIM))
TWND_R = $(patsubst %.cpp, $(OBJDIRR)/%.o, $(TWND))
TWND_D = $(patsubst %.cpp, $(OBJDIRD)/%.o, $(TWND))
PYTH_R = $(patsubst %.cpp, $(OBJDIRR)/%.o, $(PYTH))
PYTH_D = $(patsubst %.cpp, $(OBJDIRD)/%.o, $(PYTH))

EVERY_OBJ_R = $(SIM_R) $(TWND_R) $(PYTH_R)
EVERY_OBJ_D = $(SIM_D) $(TWND_D) $(PYTH_D)
DEP = $(patsubst %.o,%.o.dep, $(EVERY_OBJ_R) $(EVERY_OBJ_D))


all: dirs $(EVERY_BIN)


$(OBJDIRR)/test-tool-qt4.o: .generated/test-tool-qt4.moc
.generated/test-tool-qt4.moc: test-tool-qt4.cpp
	$(MOC) -o $@ $<

#../robot-test-tool: $(SIM_R) $(TWND_R)
#../robot-test-tool_d: $(SIM_D) $(TWND_D)


../robot-test-tool: $(SIM_R) $(TWND_R)
	$(LINK) $(LINK_OUT)$@ $(LIBS) $^ 
../robot-test-tool_d: $(SIM_D) $(TWND_D)
	$(LINK) $(LINK_OUT)$@ $(LIBS) $^ 

../cpp_household.pyd: $(SIM_R) $(PYTH_R)
	$(LINK) $(LINK_FLAGS_DLL) $(SHARED) $(BOOST_PYTHON) $^ $(LIBS) $(LINK_OUT)$@
../cpp_household.so: $(SIM_R) $(PYTH_R)
	$(LINK) $(LINK_FLAGS_DLL) $(SHARED) $(LINK_OUT)$@ $(LIBS) $(BOOST_PYTHON) $^
../cpp_household_d.$(FILEEXT): $(SIM_D) $(PYTH_D)
	$(LINK) $(LINK_FLAGS_DLL) $(SHARED) $(LINK_OUT)$@ $(LIBS) $(BOOST_PYTHON) $^

$(OBJDIRR)/%.o: %.cpp
	$(CC) $(CFLAGS) -c $<  $(MINUS_O)$@ $(DEPENDS)
$(OBJDIRD)/%.o: %.cpp
	$(CC) $(CFLAGSD) $(CFLAGSD2) -c $<  $(MINUS_O)$@ $(DEPENDS)

.PHONY: depends clean dirs

clean:
	$(RM) $(EVERY_BIN) $(EVERY_OBJ_R) $(EVERY_OBJ_D) .generated/*.moc *.ilk *.pdb $(DEP)
	rm -rf .generated
	rm -rf $(OBJDIRD)
	rm -rf $(OBJDIRR)

depends:
	cat  $(DEP) > Makefile.dep

.generated:
	mkdir -p .generated
$(OBJDIRR):
	mkdir -p $@
$(OBJDIRD):
	mkdir -p $@

dirs: .generated $(OBJDIRR) $(OBJDIRD)

-include Makefile.dep