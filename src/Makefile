SDK        = C:\Projects\3do-devkit
PROJECT    = C:\Projects\3do-math
FILESYSTEM = CD
EXENAME	   = $(FILESYSTEM)/LaunchMe
ISONAME    = madam_bench.iso
STACKSIZE  = 4096
BANNER	   = banner.bmp

CC	   = armcc
CXX	   = armcpp
AS 	   = armasm
LD	   = armlink
RM	   = rm
MODBIN     = modbin
MAKEBANNER = MakeBanner
3DOISO     = 3doiso
3DOENCRYPT = 3DOEncrypt

OPT      = -O2
CFLAGS   = $(OPT) -bi -za1 -zas1 -wn -ff -fa -d __3DO__=1 -cpu ARM6
CXXFLAGS = $(CFLAGS)
ASFLAGS	 = -BI -i $(SDK)/include/3do
INCPATH	 = -I $(SDK)/include/3do -I $(SDK)/include/ttl -I ..
LIBPATH	 = $(SDK)/lib/3do
LDFLAGS	 = -aif -reloc -ro-base 0 -libpath $(LIBPATH) -nodebug -remove -info Sizes
FOLIO_LDFLAGS = -aif -reloc -ro-base 0x00 -dupok -remove -nodebug -verbose
STARTUP	 = $(LIBPATH)/cstartup.o

LIBS = \
	$(LIBPATH)/clib.lib			\
	$(LIBPATH)/cpluslib.lib		\
	$(LIBPATH)/swi.lib			\
	$(LIBPATH)/lib3do.lib		\
	$(LIBPATH)/operamath.lib	\
	$(LIBPATH)/audio.lib		\
	$(LIBPATH)/music.lib		\
	$(LIBPATH)/filesystem.lib	\
	$(LIBPATH)/graphics.lib		\
	$(LIBPATH)/input.lib		\
	
FOLIO_LIBS = \
	$(LIBPATH)/clib.lib \
	$(LIBPATH)/../community/svc_funcs.lib \
	$(LIBPATH)/subroutinestartup.o \

SRC_S   = $(wildcard *.s)
SRC_C   = $(wildcard *.c)

OBJ += $(SRC_S:%.s=%.s.o)
OBJ += $(SRC_C:%.c=%.c.o)

all: clean launchme modbin banner super.privfolio iso run

usb: clean launchme modbin banner super.privfolio iso copy_usb run

launchme: builddir $(OBJ)
	$(LD) -dupok -o $(EXENAME) $(LDFLAGS) $(STARTUP) $(LIBS) $(addprefix build/, $(notdir $(OBJ)))

modbin:
	$(MODBIN) --stack=$(STACKSIZE) $(EXENAME) $(EXENAME)

banner:
	$(MAKEBANNER) $(BANNER) $(FILESYSTEM)/BannerScreen

iso:
	$(3DOISO) -in $(FILESYSTEM) -out ..\$(ISONAME)
	$(3DOENCRYPT) genromtags ..\$(ISONAME)

run:
	/c/RetroArch/retroarch.exe -L C:\RetroArch\cores\opera_libretro.dll $(PROJECT)\$(ISONAME)

builddir:
	mkdir -p build/

%.s.o: %.s
	$(AS) $(INCPATH) $(ASFLAGS) $< -o build/$(notdir $@)

%.c.o: %.c
	$(CC) $(INCPATH) $(CFLAGS) -c $< -o build/$(notdir $@)
	
clean:
	$(RM) -vfr build $(EXENAME) $(EXENAME).sym $(ISONAME)

build/super.c.o: folio/super.c
	$(CC) $(INCPATH) $(CFLAGS) -c $< -o build/$(notdir $@)
    
build/super.privfolio.unsigned: build/super.c.o
	$(LD) -o $@ $(FOLIO_LDFLAGS) $(FOLIO_LIBS) $<

super.privfolio.unsigned: builddir build/super.privfolio.unsigned

CD/System/Folios/super.privfolio: super.privfolio.unsigned
	$(MODBIN) --stack=$(STACKSIZE) --flags=2 --sign=3do --subsystype=1 --type=4 --name=super build/$< $@

super.privfolio: CD/System/Folios/super.privfolio

.PHONY: clean modbin banner super.privfolio iso
