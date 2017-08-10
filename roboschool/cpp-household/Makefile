UNAME := $(shell uname -s)
OBJDIRR=.build-release
OBJDIRD=.build-debug

ifeq ($(UNAME),Linux)
  PKG  =pkg-config
  MOC  =moc -qt=5
  LIBS =-L/usr/lib64 -lm -lGL -lGLU
  INC  =-I/usr/include
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
  RPATH=-Wl,-z,origin -Wl,--disable-new-dtags -Wl,-rpath,'$$ORIGIN/cpp-household/bullet_local_install/lib'
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
INC     += `$(PKG) --cflags Qt5Widgets Qt5OpenGL assimp python-$(PYTHON)`
LIBS    += -lstdc++ `$(PKG) --libs Qt5OpenGL Qt5Widgets assimp python-$(PYTHON)`
INC     += -Ibullet_local_install/include -Ibullet_local_install/include/bullet -I/usr/local/include/bullet
LIBS    += $(RPATH) -Lbullet_local_install/lib -lLinearMath -lBullet3Common -lBulletCollision -lBulletDynamics -lBulletInverseDynamics -lPhysicsClientC_API

ifeq ($(PYTHON),2.7)
    BOOST_PYTHON = -lboost_python
else
    BOOST_PYTHON = -lboost_python$(BOOST_PYTHON3_POSTFIX)
endif

CC=gcc
LINK=gcc
AR=ar r
AR_OUT=
LINK_OUT= -o
MINUS_O = -o
CFLAGS   = -std=c++11 -Wall -Wno-unused-variable -Wno-unused-function -Wno-deprecated-register -fPIC -DBT_USE_DOUBLE_PRECISION -g -O3 -march=native $(INC)
CFLAGSD  = -std=c++11 -Wall -Wno-unused-variable -Wno-unused-function -Wno-deprecated-register -fPIC -DBT_USE_DOUBLE_PRECISION -g -DDEBUG $(INC)

SHARED  = -shared
DEPENDS = -MMD -MF $@.dep

EVERY_BIN=../robot-test-tool ../robot-test-tool_d ../cpp_household.so ../cpp_household_d.so

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
	$(LINK) $(LINK_OUT)$@ $^ $(LIBS)
../robot-test-tool_d: $(SIM_D) $(TWND_D)
	$(LINK) $(LINK_OUT)$@ $^ $(LIBS)

../cpp_household.so: $(SIM_R) $(PYTH_R)
	$(LINK) $(SHARED) $(LINK_OUT)$@ $^ $(LIBS) $(BOOST_PYTHON)
../cpp_household_d.so: $(SIM_D) $(PYTH_D)
	$(LINK) $(SHARED) $(LINK_OUT)$@ $^ $(LIBS) $(BOOST_PYTHON)

$(OBJDIRR)/%.o: %.cpp
	$(CC) $(CFLAGS) -c $<  $(MINUS_O)$@ $(DEPENDS)
$(OBJDIRD)/%.o: %.cpp
	$(CC) $(CFLAGSD) -c $<  $(MINUS_O)$@ $(DEPENDS)

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
