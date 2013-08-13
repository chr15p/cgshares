#
# Copyright (C) 2012 Chris Procter
#
# This copyrighted material is made available to anyone wishing to use,
# modify, copy, or redistribute it subject to the terms and conditions
# of the GNU General Public License v.2.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation,
# Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

CC=gcc
CFLAGS=-Wall -g 
EXE=cgshares 
OBJ=cgshares.o
#HEADERS=directory.h
#MANPAGE=disktool.8
#INSTALLDIR=$(INSTALL_ROOT)/usr/sbin
#MANDIR=$(INSTALL_ROOT)/usr/share/man/man8

COMPILEFLAGS=$(shell pkg-config --cflags --libs libcgroup)

all: $(OBJ) 
	$(CC) -o $(EXE) $(COMPILEFLAGS) $(OBJ)

.c.o:
	$(CC) $(CFLAGS) -c $< $(COMPILEFLAGS) $(PPFLAGS) 
