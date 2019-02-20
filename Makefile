CC = gcc
AR = ar
LIB_SUFFIX = a
NAME = libalac
ALAC_LIB = $(NAME).$(LIB_SUFFIX)

OBJS = alac.o
BIN = alacSample
RM = rm -f


ALSA_DIR = /home/pi/alsa-lib-1.1.7/build
FAAC_DIR = /home/pi/faac/bin
LIB_DIR = ./lib

INCS = -I $(FAAC_DIR)/include -I$(ALSA_DIR)/include -I./include
LIBS = -L$(FAAC_DIR)/lib -L$(ALSA_DIR)/lib -L./lib
LIB_FLAG = -lfaac -lasound -lalac
CFLAGS = $(INCS) $(LIBS) $(LIB_FLAG) -fPIC  -Wall
SYS = posix
SO_VERSION =1
SOX_posix =so
SOX=$(SOX_$(SYS))
SO_posix =.$(SOX).$(SO_VERSION)
SO_EXT =$(SO_$(SYS))
SO_LDFLAGS = -shared -Wl,-soname,$@

all: $(BIN)

clean: 
		${RM} *.o  $(LIB_DIR)/libalac.* $(BIN) *.aac

#$(BIN): $(AR) libalac$(SO_EXT)
$(BIN): $(ALAC_LIB)
		$(CC) $@.c  -o $@ $(CFLAGS)

$(ALAC_LIB) : $(OBJS)
		@mkdir -p $(LIB_DIR)
			$(AR) -r $(LIB_DIR)/$@ $^

libalac$(SO_EXT): $(OBJS)
		@mkdir -p $(LIB_DIR)
			$(CC) $(SO_LDFLAGS) $< -o $(LIB_DIR)/$@
				ln -sf $@ $(LIB_DIR)/libalac.$(SOX)

alac.o : alac.c
		$(CC) -c $< -o $@ $(CFLAGS)

