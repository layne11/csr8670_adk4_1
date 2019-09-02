#-*-Makefile-*-
# Makefile with rules for DSP applications

############################################################
# The variable without which none of this will work
#
# BLUELAB (default from environment)
#  Specifies the directory containing your BlueLab installation. This
# is normally picked up from the BLUELAB environment variable set by
# the BlueLab installer, so you'll only need to change it when
# switching between BlueLab installations.
#
# OUTPUT
#  The basename of the DSP application.
#
# IMAGE
#  Name of the image directory; typically that of the associated VM
# VM application.
#
# ASMS
#  The source files to be assembled into the DSP application
#
# LIBS
#  The libraries to link in.
#
# DEFS
#  Any flags to define on the assembler command line.
#
# DEBUGTRANSPORT
#  SPI transport in a form suitable for passing to pscli's -trans option.
#
# LIB_SET
#  Library set to use, defaults to "sdk"
#

.PHONY : FORCE

include $(BLUELAB)/Makefile.rules

# makefile for building	images suitable for FPGAs
-include $(BLUELAB)/Makefile.dsp_fpga

SPI?=-trans '$(DEBUGTRANSPORT)'

############################################
# Work out what dsp, if any, the chip has.

ifeq (,$(HARDWARE))
shape = $(shell query_chip $(subst ',,$(subst -trans,, $(SPI))))
insist = $(if $(strip $(1)),$(1),$(error Unable to query BlueCore over SPI. Check your settings under xIDE's Debug->Transport menu.))
hardware = $(call insist,$(call lookup,CHIP_NAME,$(shape)))

# We use ":=" here to make sure we only run query_chip once
HARDWARE := $(call lookup_or_first,$(hardware),$(CHIP_NAMES))

ifeq (,$(HARDWARE))
$(error Unable to automatically identify the version of BlueCore you are using)
endif
endif

ifeq (,$(filter $(HARDWARE),$(SUPPORTED_DSP_CORES)))
$(error Refusing to build DSP code as $(HARDWARE) has no supported DSP)
endif

dsp_arch=$($(HARDWARE)_arch_dir)

FULL := image/$(OUTPUT)/$(OUTPUT).kap

################################################################################
#
# We support only kalasm3 and hence everything below assumes kalasm3
#
################################
# which library set are we using
LIB_SET ?= sdk

# Find the first library that kalasm will choose to use and depend on that
# If we can't find any, depend on the basic (and report that as missing.)
libext = pa a
libsearch = \
  $(firstword \
    $(foreach ext,$(libext), \
      $(wildcard $(BL)/kalimba/lib_sets/$(LIB_SET)/$(dsp_arch)/$(1).$(ext)) \
    ) \
    $(BL)/kalimba/lib_sets/$(LIB_SET)/$(dsp_arch)/$(1).$(firstword $(libext)) \
  )

KALASM_FLAGS = $($(HARDWARE)_kalasm3_flags) $(kalasm3flags)
KLINK_FLAGS = $($(HARDWARE)_link_flags) $(klinkflags)
ELF2KAP_FLAGS = $($(HARDWARE)_elf2kap_flags)

# get the o files
OFILE_DIR := $(OUTPUT)-objects
OFILES=$(patsubst %.asm,$(OFILE_DIR)/%.o,$(ASMS))

# common variables
OUTDIR := $(if $(OUTDIR),$(OUTDIR)/,)
# Ignore OUTDIR for now, since it may well contain spaces, which kill us
# (see B-11507, M-856)
KLIBS := $(foreach d,$(LIBS),$(call libsearch,$(d)))
KALASM_INC := -I$(BL)/kalimba/lib_sets/$(LIB_SET)/include
KALASM_INC_VM := -I$(BL)/kalimba/external_includes/vm -I$(BL)/kalimba/external_includes/firmware


# default target
build :: $(FULL)

# targets for kalasm3
ifeq (,$(filter $(HARDWARE),$(KALASM3_CORES)))
$(error Refusing to build DSP code as $(HARDWARE) is not supported by Kalasm3)
endif

# See if the override linkscripts exist, and use them
DEFAULT_LINKSCRIPT = $(if $(wildcard $(OUTPUT).$(HARDWARE).link),\
                         $(OUTPUT).$(HARDWARE).link,\
                         $(BL)/kalimba/architecture/$(HARDWARE).link)

APP_LINKSCRIPT = $(wildcard $(OUTPUT).link)

LIB_LINK_SCRIPTS = $(foreach lib,$(LIBS),\
    $(if $(wildcard $(OUTPUT).$(lib).link),\
        $(OUTPUT).$(lib).link,\
        $(BL)/kalimba/lib_sets/$(LIB_SET)/$($(HARDWARE)_arch_dir)/$(lib).link))

# All the linker control files passed to the linker.  The order here is
# significant:  Default comes first to define the architecture defaults.
# The application comes last so it can override previously defined segments.
LINKSCRIPTS := $(DEFAULT_LINKSCRIPT) $(LIB_LINK_SCRIPTS) $(APP_LINKSCRIPT)

# Need to run Kalasm3, followed by the linker, followed by elf2kap
$(OUTPUT).kap : $(OUTPUT).elf depend/$(OUTPUT).link.flags
	$(elf2kap) $(ELF2KAP_FLAGS) $< -o $@

image/$(OUTPUT)/$(OUTPUT).kap : $(OUTPUT).kap
	@$(mkdir) $(dir $@)
	$(copyfile) $< $@

$(OUTPUT).elf : $(KLIBS) $(LINKSCRIPTS) $(OFILES) depend/$(OUTPUT).link.flags
	$(link) $(KLINK_FLAGS) \
	  $(filter %.link, $^) \
	  --start-group $(filter %.o, $^) $(filter %.a, $^) $(filter %.pa, $^) --end-group \
	  -o $@
	-$(kmap) disasm datadump memusage $@ >$(basename $@).kmap
	-$(kmap) memusage $@

$(OFILES) : $(OFILE_DIR)/%.o : %.asm depend/$(OUTPUT).asm.flags
	@$(mkdir) $(dir $@)
	$(kas) $(DEFS) $(KALASM_FLAGS) $< $(KALASM_INC) $(KALASM_INC_VM) --dump-pp=$(OFILE_DIR)/$*.pp -o $@


# Record various flags
# Use -include to make these happen every time before anything else

dummy.force depend/$(OUTPUT).asm.flags depend/$(OUTPUT).link.flags depend/$(OUTPUT).kap.flags :
	@$(mkdir) depend/
	@$(recordflags) depend/$(OUTPUT).asm.flags $(DEFS) $(KALASM_FLAGS) $(KALASM_INC) $(KALASM_INC_VM)
	@$(recordflags) depend/$(OUTPUT).link.flags $(KLINK_FLAGS) $(KLIBS) $(LINKSCRIPTS) $(OFILES)
	@$(recordflags) depend/$(OUTPUT).kap.flags $(ELF2KAP_FLAGS)

clean ::
	$(del) $(FULL) $(OUTPUT).kap $(OUTPUT).elf $(OFILES) $(OFILES:.o=.pp)
	$(del) depend/$(OUTPUT).asm.flags depend/$(OUTPUT).link.flags depend/$(OUTPUT).kap.flags

-include dummy.force

