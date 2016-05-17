CC=g++
DEBUG=
IFLAGS+= -Iinclude -I. -Iinterface
LFLAGS+= -L. -Llib 
LINK+= -lboost_thread -lboost_system -ldl
CXXFLAGS= -pipe -Wall -O2

.PHONY : all make_dir

ifndef MAIN_NAME
MAIN_NAME=jfr
endif
ifndef FLOW_NAME
FLOW_NAME=$(MAIN_NAME)_flow
endif

DEFINES+= -D MAIN_NAME=\"$(MAIN_NAME)\" -D FLOW_NAME=\"$(FLOW_NAME)\"

SRC=$(shell ls src/*.cpp)
TARGET_DIR=bin
OBJ_DIR=.obj

PRO=$(MAIN_NAME)
OBJ_TMP=$(subst .cpp,.o,$(SRC))
OBJ=$(subst src,$(OBJ_DIR),$(OBJ_TMP))

all:make_dir $(PRO)
	strip $(TARGET_DIR)/$(PRO)

$(OBJ_DIR)/%.o:src/%.cpp
	$(CC) -c -o $@ $< $(DEBUG) $(IFLAGS) $(LFLAGS) $(LINK) $(CXXFLAGS) $(DEFINES)

$(PRO):$(OBJ)
	$(CC) -o $(TARGET_DIR)/$@ $^ $(DEBUG) $(IFLAGS) $(LFLAGS) $(LINK) $(CXXFLAGS) $(DEFINES)

clean:
	rm -rf $(TARGET_DIR)/$(PRO) *.o *~ $(OBJ_DIR)

make_dir:
	mkdir -p $(OBJ_DIR)
