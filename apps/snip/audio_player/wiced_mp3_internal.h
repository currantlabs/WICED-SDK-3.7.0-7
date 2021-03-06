/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

#pragma once

#ifndef  __LIBMP3_INTERNAL_H__
#define  __LIBMP3_INTERNAL_H__

#ifdef __cplusplus
extern "C" {
#endif


#include "wiced.h"
#include "platform_audio.h"
#include "audio_render.h"
#include "resources.h"

/* MP3 library includes */

#include "audio_player.h"
#include "wiced_audio_interface.h"

/******************************************************
 *                      Macros
 ******************************************************/

typedef enum {
    MP3_LOG_ERROR      = 0,
    MP3_LOG_INFO       = 1,
    MP3_LOG_DEBUG      = 2,
} wiced_mp3_log_level_t;


#define MP3_LIB_PRINT(level, arg)   if ((internal != NULL) && (internal->log_level >= (wiced_mp3_log_level_t)level))   WPRINT_LIB_INFO(arg);

/******************************************************
 *                    Constants
 ******************************************************/
#define MP3_INTERNAL_TAG_VALID                 0x43EDBA34
#define MP3_INTERNAL_TAG_INVALID               0xDEADFEEB

#define APP_QUEUE_MAX_ENTRIES                   20

#define MP3_WORKER_THREAD_PRIORITY             (WICED_DEFAULT_WORKER_PRIORITY)
#define MP3_WORKER_STACK_SIZE                  (8*1024)


#define MP3_FLAGS_TIMEOUT                      (100)  /* wait for a command before doing anything          */
#define MP3_QUEUE_PUSH_TIMEOUT                 (100)  /* ms wait for pushing packet to the queue           */
#define MP3_QUEUE_POP_TIMEOUT                    (1)  /* ms wait for popping packet from the queue         */
#define MP3_THREAD_SHUTDOWN_WAIT               (100)  /* wait for shutdown done flag */

typedef enum {
    MP3_EVENT_WORKER_THREAD_SHUTDOWN   = (1 << 0),
    MP3_EVENT_WORKER_THREAD_DONE       = (1 << 1),

    /* this starts "Decode to the end of the next frame" */
    MP3_EVENT_WORKER_DECODE_FRAME      = (1 << 2),

} MP3_EVENTS_T;

#define MP3_EVENT_WORKER_THREAD_EVENTS  (MP3_EVENT_WORKER_DECODE_FRAME  | MP3_EVENT_WORKER_THREAD_SHUTDOWN)

/******************************************************
 *                   Enumerations
 ******************************************************/

typedef enum {
    MP3_INPUT_BUFFER_FL = 0,
    MP3_INPUT_BUFFER_FR,
    MP3_INPUT_BUFFER_FC,
    MP3_INPUT_BUFFER_LFE,
    MP3_INPUT_BUFFER_BL,
    MP3_INPUT_BUFFER_BR,

    MP3_INPUT_BUFFER_MAX
} wiced_mp3_input_buffer_map;

/******************************************************
 *                 Type Definitions
 ******************************************************/

/******************************************************
 *                    Structures
 ******************************************************/


typedef struct mp3_decoder_stream_info_s {
    /* within the entire streaming file
    * currently also used for creating the pts value for the renderer
    */
    uint32_t                    data_offset;
    wiced_bool_t                end_of_input_stream;

    /* we may use one packet for 2 or more "read" callbacks */
    audio_player_packet_info_t  packet;

    /* from Metadata */
    uint64_t                    total_samples;  /* total sample in stream   */
    uint16_t                    sample_rate;    /* sample rate              */
    uint8_t                     channels;       /* number of channels       */
    uint8_t                     bps;            /* bits per sample          */

} mp3_decoder_stream_info_t;


typedef struct mp3_internal_s {

    uint32_t                        tag;

    wiced_mp3_log_level_t           log_level;

    /* app callbacks to get & push audio buffers */
    wiced_decoder_audio_buffer_get  buffer_get;   /* get a buffer to fill with LPCM and submit  */
    wiced_decoder_audio_buffer_push buffer_push;  /* push a buffer with LPCM data in it to the audio render */

    wiced_thread_t                  mp3_worker_thread;
    wiced_thread_t*                 mp3_worker_thread_ptr;
    wiced_event_flags_t             mp3_events;

    void*                           mp3_decoder;
    wiced_bool_t                    mp3_stream_inited;

    wiced_queue_t                   mp3_packet_queue;


    /* the stream we are decoding */
    mp3_decoder_stream_info_t       stream;
    wiced_audio_player_source_info_t        source_info;    /* currently playing source information (if known) */
    wiced_bool_t                    user_stop;      /* user requested stop */
    wiced_bool_t                    shut_it_down;   /* internal signal to actually deinit the stream */

        /* debug    */
    uint16_t                        debug_queue_max;        /* max count on the queue   */
    uint16_t                        debug_queue_count;      /* count stuff on the queue */

} mp3_internal_t;


/******************************************************
 *               Function Declarations
 ******************************************************/
/******************************************************
 *               Variables Definitions
 ******************************************************/
/******************************************************
 *               Function Definitions
 ******************************************************/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif  /* __LIBFLAC_INTERNAL_H__    */
