#------------------------------------------------------------------------------
# exports:
#------------------------------------------------------------------------------

export CFLAGS= -Wall -Werror
export LFLAGS= -lstrmanager -llog4cxx -lboost_thread
export bin-dir= $(basedir)/usr/bin
export cfg-dir= $(basedir)/etc/strBridge

#------------------------------------------------------------------------------
# all:
#------------------------------------------------------------------------------

all:
		make -C src

#------------------------------------------------------------------------------
# clean:
#------------------------------------------------------------------------------

clean:
		make -C src clean

#------------------------------------------------------------------------------
# install:
#------------------------------------------------------------------------------

install:
		make -C src install
		mkdir -p $(cfg-dir)
		install -m644 etc/logger.properties $(cfg-dir)