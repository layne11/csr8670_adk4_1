# This defines the config features that will be built into libraries
CONFIG_FEATURES:=

# This sets the execution modes that will be supported by the build (vm, native or assisted)
CONFIG_EXECUTION_MODES:=assisted

# Set the lib directories to exclude in the build (if unset all libraries are included)
# (Use space as a delimiter)
CONFIG_DIRS_FILTER:=chain chain_builder operators audio_mixer audio_ports \
audio_input_common audio_input_a2dp audio_input_analogue audio_input_i2s \
audio_input_common_sim audio_input_a2dp_sim audio_input_analogue_sim audio_input_i2s_sim \
operator_spy operator_list_spy  audio_mixer_spy

