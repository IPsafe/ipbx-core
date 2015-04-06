#
# Asterisk -- A telephony toolkit for Linux.
#
# Makefile for PBX frontends (dynamically loaded)
#
# Copyright (C) 1999, Mark Spencer
#
# Mark Spencer <markster@linux-support.net>
#
# $Id: Makefile,v 0.1 2005/05/14 19:49:08 mkezys Exp $
#
# This program is free software, distributed under the terms of
# the GNU General Public License
#

#
# Set ASTERISKINCDIR variable to the directory containing the sources of
# Asterisk PBX.
#
#ASTERISKINCDIR=/usr/include/asterisk
#ASTERISKINCDIR=/usr/src/asterisk/include

#
# Set ASTERISKMODDIR variable to the directory where ASTERISK's modules reside.
# The modules will be installed in this directory.
#
ASTERISKMODDIR=/usr/lib/asterisk/modules

#
# Set ASTERISKETCDIR variable to the directory where ASTERISK's configuration
# files reside. The config files will be installed in this directory.
#
ASTERISKETCDIR=/etc/asterisk
PGSQLINCLUDE = /usr/include/postgresql/
MYSQLINCLUDE = -lmysqlclient -L/usr/lib64 -L/usr/lib -I/usr/include/mysql -DBIG_JOINS=1  -fno-strict-aliasing  -g
ASTERISKINCLUDE=-I$(PGSQLINCLUDE) $(MYSQLINCLUDE) -I/usr/include/pgsql -I/usr/local/pgsql -I/usr/local/pgsql/include -I$(ASTERISKINCDIR)
#ASTERISKINCLUDE=-I$(ASTERISKINCDIR)


APPS=app_ipbx.so 
OBJ=app_ipbx.o Configuration.o Calldata.o Database.o Report.o Functions.o
CC=gcc
INSTALL=install

#Tell gcc to optimize the asterisk's code
OPTIMIZE=-O6

#Include debug symbols in the executables (-g) and profiling info (-pg)
DEBUG=-g #-pg

CFLAGS+=-D_GNU_SOURCE

CFLAGS+=$(shell if [ -f /usr/include/linux/zaptel.h ]; then echo "-DZAPTEL_OPTIMIZATIONS"; fi)
CFLAGS+=$(shell if [ -f /usr/local/include/zaptel.h ]; then echo "-DZAPTEL_OPTIMIZATIONS"; fi)

LIBEDIT=editline/libedit.a

CFLAGS+= $(DEBUG_THREADS)
CFLAGS+= $(TRACE_FRAMES)
CFLAGS+= $(MALLOC_DEBUG)
CFLAGS+= $(BUSYDETECT)
CFLAGS+= $(OPTIONS)

CFLAGS+=-fPIC
#CFLAGS+=-fPIC -lpthread -ldl -lncurses -lm

MLFLAGS +=-L /usr/local/pgsql/lib
all: $(APPS)

clean:
	rm -f *.so *.o look .depend

#$(if $(filter app_ipbx,$(EMBEDDED_MODS)),modules.link,app_ipbx.so): Calldata.o BancoDeDados.o Relatorio.o Agente.o

install: all
	mkdir -p $(DESTDIR)$(ASTERISKMODDIR)
#	mv $(DESTDIR)$(ASTERISKMODDIR)/app_ipbx.so $(DESTDIR)/usr/lib/asterisk/app_ipbx.so_old
	for x in $(APPS); do $(INSTALL) -m 755 $$x $(DESTDIR)$(ASTERISKMODDIR) ; done


%.o: %.c 
	$(CC) -pipe -g $(ASTERISKINCLUDE) $(CFLAGS) -c -o $@ $<

#app_ipbx.o: app_ipbx.c
#	$(CC) -pipe $(ASTERISKINCLUDE) $(CFLAGS) -c -o app_ipbx.o app_ipbx.c

app_ipbx.so: $(OBJ)
	$(CC) -shared -Xlinker -x -g -o $@ $(OBJ) -lpq -lz $(MLFLAGS)



ifneq ($(wildcard .depend),)
include .depend
endif

depend: .depend

.depend:
	../mkdep $(CFLAGS) `ls *.c`

env:
	env
