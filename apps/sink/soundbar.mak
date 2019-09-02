$(warning #############################################################)
$(warning            Running soundbar.mak)
$(warning #############################################################)

# ensure we always run the rules for each kalimba app to get the latest version
.PHONY : image/START.HTM \
	image/usb_root \
	image/usb_fat \
	image/cvc_headset/cvc_headset.kap \
	image/cvc_headset_2mic/cvc_headset_2mic.kap \
	image/cvc_handsfree/cvc_handsfree.kap \
	image/cvc_handsfree_2mic/cvc_handsfree_2mic.kap \
	image/one_mic_example_cvsd/one_mic_example_cvsd.kap \
	image/two_mic_example_cvsd/two_mic_example_cvsd.kap \
	image/one_mic_example_16k/one_mic_example_16k.kap \
	image/two_mic_example_16k/two_mic_example_16k.kap \
	image/sbc_decoder/sbc_decoder.kap \
	image/aptx_decoder/aptx_decoder.kap \
	image/a2dp_low_latency_1mic/a2dp_low_latency_1mic.kap \
	image/a2dp_low_latency_2mic/a2dp_low_latency_2mic.kap \
	image/faststream_decoder/faststream_decoder.kap \
	image/aptx_acl_sprint_decoder/aptx_acl_sprint_decoder.kap \
	image/mp3_decoder/mp3_decoder.kap \
	image/aac_decoder/aac_decoder.kap \
	image/spdif_sink/spdif_sink.kap \
	image/aptxhd_decoder/aptxhd_decoder.kap \
	image/config \
	prompts
     
#Check if cVc is to be included
ifneq (, $(findstring -DINCLUDE_CVC,$(DEFS)))      
    
#  1 mic cVc
#image/cvc_headset/cvc_headset.kap :
#	$(mkdir) image/cvc_headset
#	$(copyfile) ../../kalimba/apps/cvc_headset/image/cvc_headset/cvc_headset.kap $@

#image.fs : image/cvc_headset/cvc_headset.kap

# 2 mic cVc
#image/cvc_headset_2mic/cvc_headset_2mic.kap :
#	$(mkdir) image/cvc_headset_2mic
#	$(copyfile) ../../kalimba/apps/cvc_headset_2mic/image/cvc_headset_2mic/cvc_headset_2mic.kap $@

#image.fs : image/cvc_headset_2mic/cvc_headset_2mic.kap

#  1 mic Handsfree cVc
#image/cvc_handsfree/cvc_handsfree.kap :
#	$(mkdir) image/cvc_handsfree
#	$(copyfile) ../../kalimba/apps/cvc_handsfree/image/cvc_handsfree/cvc_handsfree.kap $@

#image.fs : image/cvc_handsfree/cvc_handsfree.kap

#  2 mic Handsfree cVc
#image/cvc_handsfree_2mic/cvc_handsfree_2mic.kap :
#	$(mkdir) image/cvc_handsfree_2mic
#	$(copyfile) ../../kalimba/apps/cvc_handsfree_2mic/image/cvc_handsfree_2mic/cvc_handsfree_2mic.kap $@
#
#image.fs : image/cvc_handsfree_2mic/cvc_handsfree_2mic.kap

endif

#Check if example plugins are to be included
ifneq (, $(findstring -DINCLUDE_DSP_EXAMPLES,$(DEFS)))

#image/one_mic_example_cvsd/one_mic_example_cvsd.kap :
#	$(mkdir) image/one_mic_example_cvsd
#	$(copyfile) ../../kalimba/apps/one_mic_example/image/one_mic_example_cvsd/one_mic_example_cvsd.kap $@
    
#image.fs : image/one_mic_example_cvsd/one_mic_example_cvsd.kap

#image/two_mic_example_cvsd/two_mic_example_cvsd.kap :
#	$(mkdir) image/two_mic_example_cvsd
#	$(copyfile) ../../kalimba/apps/two_mic_example/image/two_mic_example_cvsd/two_mic_example_cvsd.kap $@
    
#image.fs : image/two_mic_example_cvsd/two_mic_example_cvsd.kap

#image/one_mic_example_16k/one_mic_example_16k.kap :
#	$(mkdir) image/one_mic_example_16k
#	$(copyfile) ../../kalimba/apps/one_mic_example/image/one_mic_example_16k/one_mic_example_16k.kap $@
    
#image.fs : image/one_mic_example_16k/one_mic_example_16k.kap

#image/two_mic_example_16k/two_mic_example_16k.kap :
#	$(mkdir) image/two_mic_example_16k
#	$(copyfile) ../../kalimba/apps/two_mic_example/image/two_mic_example_16k/two_mic_example_16k.kap $@
    
#image.fs : image/two_mic_example_16k/two_mic_example_16k.kap

endif



######################################################################################################
##### A2DP DECODER VERSIONS
######################################################################################################

# copy in sbc decoder 
image/sbc_decoder/sbc_decoder.kap :
	$(mkdir) image/sbc_decoder
	$(copyfile) ../../kalimba/apps/a2dp_sink/image/sbc_decoder/sbc_decoder.kap $@

image.fs : image/sbc_decoder/sbc_decoder.kap

# copy in a2dp_low_latency_1mic decoder
#image/a2dp_low_latency_1mic/a2dp_low_latency_1mic.kap :
#	$(mkdir) image/a2dp_low_latency_1mic
#	$(copyfile) ../../kalimba/apps/a2dp_low_latency_1mic/image/a2dp_low_latency_1mic/a2dp_low_latency_1mic.kap $@

#image.fs : image/a2dp_low_latency_1mic/a2dp_low_latency_1mic.kap

# copy in a2dp_low_latency_2mic
#image/a2dp_low_latency_2mic/a2dp_low_latency_2mic.kap :
#	$(mkdir) image/a2dp_low_latency_2mic
#	$(copyfile) ../../kalimba/apps/a2dp_low_latency_2mic/image/a2dp_low_latency_2mic/a2dp_low_latency_2mic.kap $@

#image.fs : image/a2dp_low_latency_2mic/a2dp_low_latency_2mic.kap

# copy in mp3 decoder 
#image/mp3_decoder/mp3_decoder.kap :
#	$(mkdir) image/mp3_decoder
#	$(copyfile) ../../kalimba/apps/a2dp_sink/image/mp3_decoder/mp3_decoder.kap $@

#image.fs : image/mp3_decoder/mp3_decoder.kap

# copy in aac decoder 
#image/aac_decoder/aac_decoder.kap :
#	$(mkdir) image/aac_decoder
#	$(copyfile) ../../kalimba/apps/a2dp_sink/image/aac_decoder/aac_decoder.kap $@

#image.fs : image/aac_decoder/aac_decoder.kap

# copy in faststream_decoder
image/faststream_decoder/faststream_decoder.kap :
	$(mkdir) image/faststream_decoder
	$(copyfile) ../../kalimba/apps/a2dp_sink/image/faststream_decoder/faststream_decoder.kap $@

image.fs : image/faststream_decoder/faststream_decoder.kap

# copy in aptX decoder
#image/aptx_decoder/aptx_decoder.kap :
#	$(mkdir) image/aptx_decoder
#	$(copyfile) ../../kalimba/apps/a2dp_sink/image/aptx_decoder/aptx_decoder.kap $@

#image.fs : image/aptx_decoder/aptx_decoder.kap

# copy in aptx_acl_sprint_decoder
# Note: If using aptX Low Latency you should also include the aptx_decoder.kap above as well
#image/aptx_acl_sprint_decoder/aptx_acl_sprint_decoder.kap :
#	$(mkdir) image/aptx_acl_sprint_decoder
#	$(copyfile) ../../kalimba/apps/a2dp_sink/image/aptx_acl_sprint_decoder/aptx_acl_sprint_decoder.kap $@

#image.fs : image/aptx_acl_sprint_decoder/aptx_acl_sprint_decoder.kap

# copy in aptX HD decoder
#image/aptxhd_decoder/aptxhd_decoder.kap :
#	$(mkdir) image/aptxhd_decoder
#	$(copyfile) ../../kalimba/apps/a2dp_sink/image/aptxhd_decoder/aptxhd_decoder.kap $@

#image.fs : image/aptxhd_decoder/aptxhd_decoder.kap

# check if spdif is to be included
ifneq (, $(findstring -DENABLE_WIRED,$(DEFS))) 
 
image/spdif_sink/spdif_sink.kap :
	$(mkdir) image/spdif_sink
	$(copyfile) ../../kalimba/apps/a2dp_sink/image/spdif_sink/spdif_sink.kap $@

image.fs : image/spdif_sink/spdif_sink.kap

endif

######################################################################################################
##### USB FILES
######################################################################################################

# If COPY_USB_MS_README is defined
ifneq (,$(findstring -DCOPY_USB_MS_README,$(DEFS)))

# Copy START.HTM into image folder
image/START.HTM:
	$(copyfile) START.HTM $@
image.fs : image/START.HTM

# Copy USB Root info to image folder
image/usb_root:
	$(copyfile) usb_root $@
image.fs : image/usb_root

# Copy USB FAT info to image folder
image/usb_fat:
	$(copyfile) usb_fat $@
image.fs : image/usb_fat


# If COPY_USB_MS_README not defined
else

# Remove START.HTM and USB mass storage info
image.fs : | remove_usb_ms

remove_usb_ms :
	$(del) image/START.HTM
	$(del) image/usb_root
	$(del) image/usb_fat

endif

######################################################################################################
##### CONFIG FILES
######################################################################################################

# If ENABLE_FILE_CONFIG is defined
ifneq (,$(findstring -DENABLE_FILE_CONFIG,$(DEFS)))

# Copy config files into image folder
image.fs : image/config

image/config:
	 $(mkdir) image/config
	 xcopy config\\soundbar\\*.snk image\\config /Y    

# If ENABLE_FILE_CONFIG not defined
else

# Remove config files
image.fs : | remove_config

remove_config :
	$(del) image/config/*

endif

#####################################################################################################
##### if DSP is enabled show message warning that a cVc key will be required to hear audio 
#####################################################################################################
ifneq (, $(findstring -DINCLUDE_CVC,$(DEFS))) 
    $(warning #############################################################)
    $(warning A cVc license key must be used before SCO audio will be heard)
    $(warning #############################################################)
endif

#####################################################################################################
##### Set up prompts based on configuration
#####################################################################################################

PROMPT := $(shell cat ./image/audio_prompt_config.csr)
image.fs : prompts

prompts :
ifneq ("$(wildcard ./$(OUTPUT)_prompts/.)","")
#Prompts are defined for this project
ifneq ("$(wildcard ./image/.)","")
	$(warning $(PROMPT))
#existing image directory detected - check if default
ifeq ("$(PROMPT)", "Default")
#Using default prompts so update audio prompt files
	@$(warning Using Default prompts in image directory. Updating prompts from ./$(OUTPUT)_prompts to images/*)
	$(clean_audio_prompts)
	$(create_audio_prompt_dirs)
	$(copy_audio_prompts)
else
# do nothing - Customer is using customised audio prompts
	$(warning Non default voice prompts detected. Not updating.)
endif	
else
# ./image/prompts does not exist, Copy in all audio prompts files
	@$(warning No prompts exist in image directory. Copying default prompts from ./$(OUTPUT)_prompts to images/*)
	@xcopy /Q /Y .\\$(OUTPUT)_prompts\\*.csr .\\image\\
	$(create_audio_prompt_dirs)
	$(copy_audio_prompts)
endif
else
	$(warning No voice prompts defined for this application)
endif

define clean_audio_prompts
	@$(del) ./image/prompts  # delete any existing voice prompts
	@$(del) ./image/headers  # delete any existing voice prompt headers
	@$(del) ./image/refname  # delete any existing voice prompt refname
endef

define create_audio_prompt_dirs
	@$(warning Creating Audio prompts directories )
	@$(mkdir) ./image/prompts
	@$(mkdir) ./image/headers
	@$(mkdir) ./image/refname
endef

define copy_audio_prompts
	@xcopy /Q /Y .\\$(OUTPUT)_prompts\\*.prm .\\image\\prompts 
	@xcopy /Q /Y .\\$(OUTPUT)_prompts\\*.idx .\\image\\headers
	@xcopy /Q /Y .\\$(OUTPUT)_prompts\\*.txt .\\image\\refname 
endef

