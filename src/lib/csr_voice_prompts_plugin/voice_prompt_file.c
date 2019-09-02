/****************************************************************************
Copyright (c) 2016 Qualcomm Technologies International, Ltd.
Part of ADK 4.1
 
FILE NAME
    voice_prompt_file.c
 
DESCRIPTION
     Voice prompt file related functions.
*/
#include <stdio.h>
#include <string.h>

#include <source.h>
#include <file.h>

#include "print.h"

#include "voice_prompts_utils.h"
#include "voice_prompt_file.h"

#define SIZE_PROMPT_DATA   (12)

typedef struct
{
    uint16                     no_prompts;
    uint16                     no_prompts_per_lang;
} voice_prompts_header;

static voice_prompts_header     header;

static void setPropertiesForTone(bool is_stereo, voice_prompt_t* prompt);
static Source getToneSource(const ringtone_note * tone);
static Source getVoicePromptSource(unsigned file_index);

/****************************************************************************
DESCRIPTION
    Set the number of voice prompts and the number of languages.
    This is used only during initialisation.
*/
bool VoicePromptsFileSetVpPerLanguage(uint16 prompts_number, unsigned languages_number)
{
    header.no_prompts = prompts_number;
    header.no_prompts_per_lang = (uint16)(prompts_number / languages_number);

    /* There is a problem with the number of prompts per language.
       There should be an integer number of prompts per language,
       prompts should not be duplicated across language sets. */
    return (prompts_number % languages_number) ? FALSE : TRUE;
}

/****************************************************************************
DESCRIPTION
    Get voice prompt index based on its id and language.
*/
unsigned VoicePromptsFileGetIndex(unsigned id, unsigned language)
{
    unsigned index = id;
    index += language * header.no_prompts_per_lang;

    /* Sanity checking */
    if(index >= header.no_prompts)
    {
        Panic();
    }

    PRINT(("VP: Play prompt %d of %d\n", index+1, header.no_prompts));

    return index;
}

/****************************************************************************
DESCRIPTION
    Set up prompt structure content using voice prompt header file pointed by
    file_index.
*/
void VoicePromptsFileSetProperties(unsigned file_index, voice_prompt_t* prompt)
{
    char file_name[17];
    Source lSource = NULL;
    const uint8* rx_array;

    /* Get the header file name */
    sprintf(file_name, "headers/%d.idx", file_index);
    lSource = StreamFileSource(FileFind(FILE_ROOT, file_name, (uint16)strlen(file_name)));

    /* Check source created successfully */
    if(SourceSize(lSource) < SIZE_PROMPT_DATA)
    {
        /* Finished with header source, close it */
        SourceClose(lSource);
        Panic();
    }

    rx_array = SourceMap(lSource);

    prompt->stereo        = rx_array[4];
    prompt->codec_type = rx_array[9];
    prompt->playback_rate = (uint16)(((uint16)rx_array[10] << 8) | (rx_array[11]));

    if(!VoicePromptsIsCodecTypeValid(prompt->codec_type))
    {
        Panic();
    }

    /* Finished with header source, close it */
    if(!SourceClose(lSource))
    {
        Panic();
    }
}

/****************************************************************************
DESCRIPTION
    Get tone or voice prompt source and set up prompt structure.
*/
Source VoicePromptsFileGetToneOrPrompt(const vp_context_t * pData, voice_prompt_t* prompt,
        bool tones_require_source)
{
    Source audio_source;

    if(!pData || !prompt )
        return NULL;

    /* determine if this is a tone */
    if(pData->tone)
    {
        setPropertiesForTone(pData->features.stereo, prompt);
        if(tones_require_source)
        {
            audio_source = getToneSource(pData->tone);
        }
        else
        {
            audio_source = NULL;
        }
    }
    else
    {
        unsigned file_index = VoicePromptsFileGetIndex(pData->prompt_id, pData->language);
        VoicePromptsFileSetProperties(file_index, prompt);
        audio_source = getVoicePromptSource(file_index);
    }

    PRINT(("Prompt: %X rate 0x%x stereo %u\n", prompt->codec_type,prompt->playback_rate,prompt->stereo));

    return audio_source;
}

/****************************************************************************
DESCRIPTION
    Set up prompt structure for tone.
*/
static void setPropertiesForTone(bool is_stereo, voice_prompt_t* prompt)
{
    prompt->codec_type = voice_prompts_codec_tone;
    prompt->playback_rate = 8000;
    prompt->stereo = is_stereo;
}

/****************************************************************************
DESCRIPTION
    Get tone source.
*/
static Source getToneSource(const ringtone_note * tone)
{
    PRINT(("VP: Prompt is a tone 0x%p\n", (void*)tone));

    return StreamRingtoneSource(tone);
}

/****************************************************************************
DESCRIPTION
    Get voice prompt source.
*/
static Source getVoicePromptSource(unsigned file_index)
{
    char file_name[17];

    sprintf(file_name, "prompts/%d.prm", file_index);

    PRINT(("File Prompt: %s\n", file_name));

    return StreamFileSource(FileFind(FILE_ROOT, file_name, (uint16)strlen(file_name)));
}
