#
# (C) Copyright 2009
# Texas Instruments, <www.ti.com>
#
# (C) Copyright 2000, 2001, 2002
# Wolfgang Denk, DENX Software Engineering, wd@denx.de.
#
# See file CREDITS for list of people who contributed to this
# project.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# version 2 as published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston,
# MA 02111-1307 USA
#

include $(TOPDIR)/config.mk

LIB	= lib$(BOARD).a

OBJS	:= p2.o mem.o syslib.o sys_info.o clock.o mmc.o spi.o p2_muic.o gpio.o ifx.o charging_ic.o \
	   p2_dispc.o p2_dsi.o lge_nvdata_emmc.o sparse.o omap4.o lglogo.o web_download_eng.o
SOBJS	:= lowlevel_init.o

$(LIB):	$(OBJS) $(SOBJS)
	$(AR) crv $@ $^

lglogo.o:
	$(OBJCOPY) -I binary -O elf32-littlearm -B arm --redefine-sym _binary_$(patsubst %.o,%_rle_start,$@)=$(patsubst %.o,%,$@) $(patsubst %.o,%.rle,$@) $@

web_download.o:
	$(OBJCOPY) -I binary -O elf32-littlearm -B arm --redefine-sym _binary_$(patsubst %.o,%_rle_start,$@)=$(patsubst %.o,%,$@) $(patsubst %.o,%.rle,$@) $@

web_download_eng.o:
	$(OBJCOPY) -I binary -O elf32-littlearm -B arm --redefine-sym _binary_$(patsubst %.o,%_rle_start,$@)=$(patsubst %.o,%,$@) $(patsubst %.o,%.rle,$@) $@

clean:
	rm -f $(SOBJS) $(OBJS)

distclean:	clean
	rm -f $(LIB) core *.bak .depend

#########################################################################

.depend:	Makefile $(SOBJS:.o=.S) $(OBJS:.o=.c)
		$(CC) -M $(CPPFLAGS) $(SOBJS:.o=.S) $(OBJS:.o=.c) > $@

-include .depend

#########################################################################
