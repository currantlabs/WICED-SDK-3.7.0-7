/*
 * Broadcom Proprietary and Confidential. Copyright 2016 Broadcom
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 */

/** @file
 *
 * Rf Monitoring Application
 *
 * Features demonstrated
 *  - Channel Congestion
 *
 * This application snippet continually scans channels for congestion levels looking
 * for the least congested channel.  When a channel that is at least DELTA better than the
 * current channel, we will switch to it.
 *
 * Application Instructions
 *   Connect a PC terminal to the serial port of the WICED Eval board,
 *   then build and download the application as described in the WICED
 *   Quick Start Guide
 *
 *
 */

#include <stdlib.h>
#include "wiced.h"
#include "internal/wwd_sdpcm.h"
#include "internal/wiced_internal_api.h"
#include "network/wwd_buffer_interface.h"
#include "audio/apollo/apollocore/apollo_report.h"

#ifdef INCLUDE_DISPLAY
#include "u8g_arm.h"
#endif

typedef struct
{
    char *cmd;
    uint32_t event;
} cmd_lookup_t;

int rfmon_console_command(int argc, char *argv[]);
#define RFMON_CONSOLE_COMMANDS \
    { (char*) "csa",         rfmon_console_command, 1, NULL, NULL, (char *)"channel", (char *)"Send CSA to indicated channel" }, \
    { (char*) "csa_on",      rfmon_console_command, 0, NULL, NULL, (char *)"", (char *)"Enable actual CSA" }, \
    { (char*) "csa_off",     rfmon_console_command, 0, NULL, NULL, (char *)"", (char *)"Disable CSA" }, \
    { (char*) "rf_status",  rfmon_console_command, 0, NULL, NULL, (char *)"", (char *)"Get CSA status" }, \
    { (char*) "csa_history", rfmon_console_command, 0, NULL, NULL, (char *)"", (char *)"Dump CSA history" }, \
    { (char*) "rf_logging", rfmon_console_command, 0, NULL, NULL, (char *)"", (char *)"Set individual stats packet reporting" }, \
    { (char*) "rf_thresh", rfmon_console_command, 0, NULL, NULL, (char *)"", (char *)"Set rfmon stats thresholds" }, \
    { (char*) "rf_blacklist", rfmon_console_command, 0, NULL, NULL, (char *)"", (char *)"Show rfmon blacklist" }, \

typedef enum
{
    RFMON_CONSOLE_CMD_CSA = 0,
    RFMON_CONSOLE_CMD_ON,
    RFMON_CONSOLE_CMD_OFF,
    RFMON_CONSOLE_CMD_STATUS,
    RFMON_CONSOLE_CMD_HISTORY,
    RFMON_CONSOLE_LOGGING,
    RFMON_CONSOLE_THRESHOLD,
    RFMON_CONSOLE_BLACKLIST,
    RFMON_CONSOLE_CMD_MAX,
} RFMON_CONSOLE_CMDS_T;

static cmd_lookup_t command_lookup[RFMON_CONSOLE_CMD_MAX] =
{
    { "csa",        0  },
    { "csa_on",     0  },
    { "csa_off",    0  },
    { "rf_status", 0  },
    { "csa_history", 0  },
    { "rf_logging", 0  },
    { "rf_thresh", 0  },
    { "rf_blacklist", 0  },
};

#ifdef USE_CONSOLE
#include "command_console_commands.h"
static char line_buffer[MAX_LINE_LENGTH];
static char history_buffer_storage[MAX_LINE_LENGTH * MAX_HISTORY_LENGTH];
static const command_t commands[] =
{
    RFMON_CONSOLE_COMMANDS
    ALL_COMMANDS
    CMD_TABLE_END
};
#undef PRINT_DEBUG  /* Disable debug print when using console */
#endif

/******************************************************
 *                      Macros
 ******************************************************/
/* Score range is 0 - 255 (8 bit).
   Max bar height is ~50 pixels.
   Rarely do scroes go above 200 (never have seen it)
   So map 0-200 => 0-50 by dividing by 4.
*/
#define MAP_BAR_HEIGHT(x)   (x >> 2)

#define WL_CHANSPEC_CHAN_MASK        0x00ff
#define CHSPEC_CHANNEL(chspec)    ((uint8_t)((chspec) & WL_CHANSPEC_CHAN_MASK))
#define CH20MHZ_CHSPEC(channel)    (chanspec_t)((chanspec_t)(channel) | WL_CHANSPEC_BW_20 | \
                WL_CHANSPEC_CTL_SB_NONE | (((channel) <= CH_MAX_2G_CHANNEL) ? \
                WL_CHANSPEC_BAND_2G : WL_CHANSPEC_BAND_5G))

#define MAX_SCORE   250 /* For normalizing scores */
#define PERCENT(x) (((x)*100)/MAX_SCORE)    /* Convert score to percent */

/* Experimentation (in bytes/sec) shows:
 * 6 channel (576KBytes/sec) moved score by 110.
 * 2 channel (192Kbytes) moved score by ~ 40.
 * 2 channel (176Kbytes) moved score by ~35-40.
 * Simple mappin of /5 works: (ie 576/5 ~= 110)
 * To convert from bytes to bits/sec multiply * 8 (5 * 8 = 40).
 * 40K == 40000.
 */
#define LOAD_TO_SCORE(load) (int)(load/40000)

/******************************************************
 *                    Constants
 ******************************************************/
#define DURATION    50  /* Measurement period in millisecs */
#define NUM_ITERS   2   /* How many iterations of data collection to smooth out data */
#define DELTA       20  /* New channel must be at least DELTA better than current channel */
#define INVALID_IDX -1  /* Invalid index */
#define LOG_SIZE    16   /* Number of entries in history */

#define SSID_FMT_BUF_LEN (4*32+1)   /* Length for SSID format string */

#define NUM_CSA_BCN 18   /* Send this many CSA beacons, usually 100 ms per beacon */
#define MIN_SECS    5   /* Minimum seconds between channel changes.  If this is too small, we get reports from
                           the previous (poor) channel causing us to think new xhannel is also bad. */
#define MIN_REPORTS  4   /* Wait at least this number of reports before attempting another chan switch */
#define FORMAT_CHECK_SECS    10  /* Check audio format this often */
#define BLACKLIST_TIME   1000 * 60     /* 2 Minutes */
#define LOST_SECS   10    /* Sink is 'Lost' after ths many secs of no contact */

#define BSS_BUFLEN  WLC_IOCTL_SMLEN  /* Buffer size for BSS_INFO */

#define MACDBG "%02x:%02x:%02x:%02x:%02x:%02x"
#define MAC2STRDBG(ea) (ea)[0], (ea)[1], (ea)[2], (ea)[3], (ea)[4], (ea)[5]
#define MACADDR_SZ 6

#define APP_NAME "rfmon"
#define APOLLO_MULTICAST_IPV4_ADDRESS_DEFAULT MAKE_IPV4_ADDRESS(224, 0, 0, 55)

/* Graphics stuff */
#define BAR_WIDTH   10   /* Width of the gfx bar */
#define BAR_GAP     3   /* Gap between bars */

#define BAR_BASE        51  /* Leave room underneath for channel labels */
#define HEADER_HEIGHT   9   /* Leave this space for the header */
#define BOTTOM          63  /* Absolute bottom of scrren */


/* Statistics and other Reports */
#define MAX_DEVICES 8   /* Max num of Sinks we can accept reports from */
#define MA_WIN_SZ   8   /* Moving average Window size */
#define INVALID_PERCENT     200

/* NOTE: These represent packet loss PERCENTs, not number of packets */
#define INSTANT_AUD_THRESH 10   /* Single report on this device > than this triggers */
#define MOVING_AUD_THRESH  5    /* Moving avg of this device > than this triggers */
#define ALL_AUD_THRESH     5    /* Avg of all device's Moving avg > than this triggers */

#define INSTANT_RTP_THRESH 10   /* Same as AUD packets */
#define MOVING_RTP_THRESH  5
#define ALL_RTP_THRESH     5

/******************************************************
 *                   Enumerations
 ******************************************************/
/* Reason to switch channels */
enum {
OVER_INSTANT_AUD_THRESH = 1,
OVER_MOVING_AUD_THRESH,
OVER_ALL_AUD_THRESH,
OVER_INSTANT_RTP_THRESH,
OVER_MOVING_RTP_THRESH,
OVER_ALL_RTP_THRESH,
USER_OVERRIDE,
};

enum {
PROCESS_REPORTS = 0,
FLUSH_REPORTS,
};
/******************************************************
 *                 Type Definitions
 ******************************************************/
typedef struct {
    uint16_t bits;
    uint32_t rate;
    int channels;
    int score;
} aud_format;

/* Describe the reason for CSA and who caused it */
typedef struct {
    int reason;
    uint8_t mac_addr[MACADDR_SZ];
    uint32_t speaker_channel;
} csa_cause_t;


/******************************************************
 *                    Structures
 ******************************************************/
/* Logging */
struct logger {
    int reason;
    int channel;
    uint32_t speaker_channel;
    uint8_t mac_addr[MACADDR_SZ];
    wiced_time_t time;
};
struct logger history[LOG_SIZE];

typedef struct {
    uint8_t mac_addr[MACADDR_SZ];
    uint64_t rtp_packets_received;
    uint64_t rtp_packets_dropped;
    uint64_t audio_frames_played;
    uint64_t audio_frames_dropped;
    uint num_rpts_rx;       /* number of report frames since chan switch */
    uint32_t speaker_channel;
    int ma_idx;                 /* index for moving average buffers */
    uint8_t rtp_drop_rate_buf[MA_WIN_SZ];
    uint8_t aud_drop_rate_buf[MA_WIN_SZ];
    int rssi_buf[MA_WIN_SZ];
    int last_rssi;
    int last_rtp_drop_rate;
    int last_aud_drop_rate;
    int rtp_ma;
    int aud_ma;
    int rssi_ma;
    wiced_time_t time;
} apollo_stats_t;

apollo_stats_t apollo_stats[MAX_DEVICES];

struct {
    int instant_aud_thresh; /* Single report on this device > than this triggers */
    int moving_aud_thresh;  /* Moving avg of this device > than this triggers */
    int all_aud_thresh;     /* Avg of all device's Moving avg > than this triggers */

    int instant_rtp_thresh; /* Same as AUD packets */
    int moving_rtp_thresh;
    int all_rtp_thresh;
} rf_thresh;

struct  {
    uint num_csas;  /* Count number of csas sent */
    uint manual;
    uint too_soon;
    uint moving_aud_thresh;
    uint moving_rtp_thresh;
    uint instantaneous_aud_thresh;
    uint instantaneous_rtp_thresh;
    uint all_aud_thresh;
    uint all_rtp_thresh;
} rf_counters;

typedef struct speaker_channel_name_lookup_s {
        uint32_t channel;
        char     *name;
} speaker_channel_name_lookup_t;

typedef enum {
    CHANNEL_MAP_NONE  = 0,          /* None or undefined    */
    CHANNEL_MAP_FL    = (1 << 0),   /* Front Left           */
    CHANNEL_MAP_FR    = (1 << 1),   /* Front Right          */
    CHANNEL_MAP_FC    = (1 << 2),   /* Front Center         */
    CHANNEL_MAP_LFE1  = (1 << 3),   /* LFE-1                */
    CHANNEL_MAP_BL    = (1 << 4),   /* Back Left            */
    CHANNEL_MAP_BR    = (1 << 5),   /* Back Right           */
    CHANNEL_MAP_FLC   = (1 << 6),   /* Front Left Center    */
    CHANNEL_MAP_FRC   = (1 << 7),   /* Front Right Center   */
    CHANNEL_MAP_BC    = (1 << 8),   /* Back Center          */
    CHANNEL_MAP_LFE2  = (1 << 9),   /* LFE-2                */
    CHANNEL_MAP_SIL   = (1 << 10),  /* Side Left            */
    CHANNEL_MAP_SIR   = (1 << 11),  /* Side Right           */
    CHANNEL_MAP_TPFL  = (1 << 12),  /* Top Front Left       */
    CHANNEL_MAP_TPFR  = (1 << 13),  /* Top Front Right      */
    CHANNEL_MAP_TPFC  = (1 << 14),  /* Top Front Center     */
    CHANNEL_MAP_TPC   = (1 << 15),  /* Top Center           */
    CHANNEL_MAP_TPBL  = (1 << 16),  /* Top Back Left        */
    CHANNEL_MAP_TPBR  = (1 << 17),  /* Top Back Right       */
    CHANNEL_MAP_TPSIL = (1 << 18),  /* Top Side Left        */
    CHANNEL_MAP_TPSIR = (1 << 19),  /* Top Side Right       */
    CHANNEL_MAP_TPBC  = (1 << 20),  /* Top Back Center      */
    CHANNEL_MAP_BTFC  = (1 << 21),  /* Bottom Front Center  */
    CHANNEL_MAP_BTFL  = (1 << 22),  /* Bottom Front Left    */
    CHANNEL_MAP_BTFR  = (1 << 23)   /* Bottom Front Right   */
} AUDIO_CHANNEL_MAP_T;

static speaker_channel_name_lookup_t speaker_channel_name[] =
{
        { CHANNEL_MAP_NONE  , "NONE"   },  /* None or undefined    */
        { CHANNEL_MAP_FL    , "FL"     },  /* Front Left           */
        { CHANNEL_MAP_FR    , "FR"     },  /* Front Right          */
        { CHANNEL_MAP_FC    , "FC"     },  /* Front Center         */
        { CHANNEL_MAP_LFE1  , "LFE1"   },  /* LFE-1                */
        { CHANNEL_MAP_BL    , "BL"     },  /* Back Left            */
        { CHANNEL_MAP_BR    , "BR"     },  /* Back Right           */
        { CHANNEL_MAP_FLC   , "FLC"    },  /* Front Left Center    */
        { CHANNEL_MAP_FRC   , "FRC"    },  /* Front Right Center   */
        { CHANNEL_MAP_BC    , "BC"     },  /* Back Center          */
        { CHANNEL_MAP_LFE2  , "LFE2"   },  /* LFE-2                */
        { CHANNEL_MAP_SIL   , "SIL"    },  /* Side Left            */
        { CHANNEL_MAP_SIR   , "SIR"    },  /* Side Right           */
        { CHANNEL_MAP_TPFL  , "TPFL"   },  /* Top Front Left       */
        { CHANNEL_MAP_TPFR  , "TPFR"   },  /* Top Front Right      */
        { CHANNEL_MAP_TPFC  , "TPFC"   },  /* Top Front Center     */
        { CHANNEL_MAP_TPC   , "TPC"    },  /* Top Center           */
        { CHANNEL_MAP_TPBL  , "TPBL"   },  /* Top Back Left        */
        { CHANNEL_MAP_TPBR  , "TPBR"   },  /* Top Back Right       */
        { CHANNEL_MAP_TPSIL , "TPSIL"  },  /* Top Side Left        */
        { CHANNEL_MAP_TPSIR , "TPSIR"  },  /* Top Side Right       */
        { CHANNEL_MAP_TPBC  , "TPBC"   },  /* Top Back Center      */
        { CHANNEL_MAP_BTFC  , "BTFC"   },  /* Bottom Front Center  */
        { CHANNEL_MAP_BTFL  , "BTFL"   },  /* Bottom Front Left    */
        { CHANNEL_MAP_BTFR  , "BTFR"   },  /* Bottom Front Right   */
        { 0  , NULL   },                /* End   */

};

static char *
speaker_to_name(int speaker_channel, char *buf, int buf_len)
{
    speaker_channel_name_lookup_t *entry;
    if (!buf)
        return NULL;
    *buf = 0;
    for (entry = speaker_channel_name; entry->name; entry++) {
        if (speaker_channel & entry->channel) {
            strncat(buf, entry->name, buf_len-strlen(buf)-1);
            strncat(buf, " ", buf_len-strlen(buf)-1);
        }
    }
    return buf;
}

/******************************************************
 *               Function Declarations
 ******************************************************/
static int get_channel_score(int channel, int *score);
static int pick_best(const int channels[], int scores[], int cur_chan_idx);
static void update_channel_log(int channel, csa_cause_t *cause);
static void dump_channel_log();
extern uint8_t* host_buffer_get_current_piece_data_pointer(wiced_buffer_t buffer );
extern void host_buffer_release(wiced_buffer_t buffer, wwd_buffer_dir_t direction );
static wiced_result_t get_bss_info();
static int channel_to_idx(int channel);
static wwd_result_t send_csa(int chan_idx);
static char *get_formatted_time(wiced_time_t *time, char *buf);
#ifdef INCLUDE_DISPLAY
static void draw_scores(u8g_t *u8g, const int *channels, int *scores);
static void make_page(u8g_t *u8g, const int *channels, int *scores);
static void u8g_prepare(u8g_t* u8g);
#endif

static wiced_result_t sniff_report();
static void clear_all_report_stats();
/******************************************************
 *               Variable Definitions
 ******************************************************/
#define NUM_CHANNELS 9
const int channels[NUM_CHANNELS] = {
        36, 40, 44, 48,
        149, 153, 157, 161, 165};

struct {
    int report_logging;        /* Per-packet printout of each stats report */
    int skip_csa;              /* User can disable switching (ie for debugging */
    int winner_override_idx;   /* Store user override for channel selection. */
} user_pref;

int g_have_audio = 0;

wiced_udp_socket_t glob_stats_socket;

int g_scores[NUM_CHANNELS];
int prev_scores[NUM_CHANNELS];

struct chan_info_t {
    wiced_time_t blacklist;
};
struct chan_info_t chan_info[NUM_CHANNELS];

int log_index = 0;
struct {
    int avg;
    int low;
    int worst; /* index of worst (highest) score */
} congest_scores;

aud_format g_aud_format;           /* hold current audio format */
char bss_ssid[SSID_FMT_BUF_LEN];   /* from GET_BSS_INFO */
int bss_chan_idx;                 /* from GET_BSS_INFO */
int g_best_index = 0;               /* Index of channel with best score */
int g_best_score;                   /* Current actual best score */
int g_current_chan_idx = INVALID_IDX;
int g_current_score;

/* global RTP averages */
int g_all_avg_aud;
int g_all_avg_rtp;
int g_all_avg_rssi;

#ifdef INCLUDE_DISPLAY
wiced_i2c_device_t oled_display =
{
    .port          = WICED_I2C_2,
    .address       = 0x3C,
    .address_width = I2C_ADDRESS_WIDTH_7BIT,
    .flags         = 0,
    .speed_mode    = I2C_HIGH_SPEED_MODE,
};
#endif

/******************************************************
 *               Function Definitions
 ******************************************************/

void application_start( )
{
    wiced_result_t result;
    wiced_time_t time_now, time_last = 0;
    uint32_t tmp;
    wiced_ip_address_t          mrtp_ip_address;
#ifdef INCLUDE_DISPLAY
    u8g_t u8g;
    u8g_init_wiced_i2c_device(&oled_display);
    u8g_InitComFn(&u8g, &u8g_dev_ssd1306_128x64_i2c, u8g_com_hw_i2c_fn);
#endif
    csa_cause_t cause;

    wiced_init( );

#ifdef USE_CONSOLE
    /* Run the main application function */
    command_console_init( STDIO_UART, MAX_LINE_LENGTH, line_buffer, MAX_HISTORY_LENGTH, history_buffer_storage, " " );
    console_add_cmd_table( commands );
#endif

    wwd_wifi_set_iovar_value( IOVAR_STR_MPC, 0, WWD_STA_INTERFACE );

    wwd_wifi_set_iovar_value( IOVAR_STR_IBSS_JOIN, 1, WWD_STA_INTERFACE );
    wwd_wifi_get_iovar_value( IOVAR_STR_IBSS_JOIN, &tmp, WWD_STA_INTERFACE );
    if (tmp != 1) {
        WPRINT_APP_INFO(("rfmon: Unable to set IBSS_join\n"));
    }

    memset(history, 0, sizeof(history));

    /* Fails if there is no DHCP Server to give us address */
    result = wiced_network_up(WICED_STA_INTERFACE, WICED_USE_EXTERNAL_DHCP_SERVER, NULL);
    if( result != WICED_SUCCESS ) {
        WPRINT_APP_INFO(("rfmon: Bringing up network interface failed!\n"));
    }

    /* Join multicast */
    SET_IPV4_ADDRESS(mrtp_ip_address, APOLLO_MULTICAST_IPV4_ADDRESS_DEFAULT);
    WPRINT_APP_INFO(("Joining multicast group %d.%d.%d.%d\n",
        (int)((mrtp_ip_address.ip.v4 >> 24) & 0xFF), (int)((mrtp_ip_address.ip.v4 >> 16) & 0xFF),
        (int)((mrtp_ip_address.ip.v4 >> 8) & 0xFF),  (int)(mrtp_ip_address.ip.v4 & 0xFF)));

    result = wiced_multicast_join(WICED_STA_INTERFACE, &mrtp_ip_address);
    if (result != WICED_SUCCESS) {
        WPRINT_APP_INFO(("%s: Unable to join multicast\n", __FUNCTION__));
    }

    if ((result = wiced_udp_create_socket(&glob_stats_socket, APOLLO_REPORT_PORT, WICED_STA_INTERFACE)) != WICED_SUCCESS)
    {
        WPRINT_APP_INFO(("%s: Unable to create stats socket, err %d\n", __FUNCTION__, result));
    }

    user_pref.report_logging = 0;
    user_pref.skip_csa = 0;
    user_pref.winner_override_idx = -1;

    /* Initialize thresholds to defaults */
    rf_thresh.instant_aud_thresh = INSTANT_AUD_THRESH;
    rf_thresh.moving_aud_thresh  = MOVING_AUD_THRESH;
    rf_thresh.all_aud_thresh     = ALL_AUD_THRESH;
    rf_thresh.instant_rtp_thresh = INSTANT_RTP_THRESH;
    rf_thresh.moving_rtp_thresh  = MOVING_RTP_THRESH;
    rf_thresh.all_rtp_thresh     = ALL_RTP_THRESH;

    if (user_pref.skip_csa)
        WPRINT_APP_INFO(("CSA's are disabled.\n"));

    while (1)
    {
        int score;
        int err, i;
        int iters;          /* measure this many times to smooth out counters */
        int just_switched = 0;


        memcpy(prev_scores, g_scores, sizeof(g_scores));
        memset(g_scores, 0, sizeof(g_scores));
        for (i = 0; i < NUM_CHANNELS; i++) {
            for (iters = 0; iters < NUM_ITERS; iters++) {
                err = get_channel_score(channels[i], &score);
                if (err == WWD_SUCCESS) {
                    g_scores[i] += score;
                } else {
                    g_scores[i] = -1;
                }
            }
        }

        /* Normalize the scores for multiple iterations */
        if (NUM_ITERS > 1) {
            for (i = 0; i < NUM_CHANNELS; i++) {
                if (g_scores[i] > 0) {
                    g_scores[i] /= NUM_ITERS;
                }
            }
        }

        if ( wiced_network_is_up(WICED_STA_INTERFACE) == WICED_FALSE ) {
                WPRINT_APP_INFO(("network_is_up() returns false, attempt to rejoin\n"));
                result = wiced_join_ap( );
                if (result != WICED_SUCCESS) {
                    WPRINT_APP_INFO(("wiced_join_ap() fails\n"));
                }
                WPRINT_APP_INFO(("wiced_join_ap() success\n"));
                wiced_rtos_delay_milliseconds(100);
        }
        if (get_bss_info() != WICED_SUCCESS) {
            WPRINT_APP_INFO(("get_bss_info() failed\n"));
        }

        /* Initiaize cur_chan_index if needed */
        if (g_current_chan_idx != bss_chan_idx) {
            g_current_chan_idx = bss_chan_idx;
        }
        g_current_score = g_scores[g_current_chan_idx]; /* stash this so its consistent when its printed
                                                         from the 'rf_status' command */

        wiced_time_get_time(&time_now);

        /* Need at least MIN_SECS & MIN_REPORTS before paying attention to reports,
         * otherwise we could be still processing reports from old (bad) channel */
        memset(&cause, 0, sizeof(csa_cause_t));
        if (just_switched && (time_now - time_last) < (1000 * MIN_SECS)) {
            sniff_report(&cause, FLUSH_REPORTS);
        } else {
            just_switched = 0;
            sniff_report(&cause, PROCESS_REPORTS);
        }

        /* If reports from sinks tell us to switch AND csas aren't disabled.
         * OR user is overriding */
        if ((cause.reason && !user_pref.skip_csa) ||
            (user_pref.winner_override_idx >= 0 && (user_pref.winner_override_idx != g_current_chan_idx))) {

            if (just_switched && cause.reason)
                    WPRINT_APP_INFO(("!!!just_switched && csa_cause.reason!!!\n"));

            /* About to switch channels, so pending reports can be and should be
             * tossed to reclaim buffers. */
            sniff_report(&cause, FLUSH_REPORTS);

            /* Allow at least MIN_SECS seconds between CSAs
               but allow manual override immediatly.
             */
            {
                char time_buf[16];
                (void)time_buf; /* Avoid compiler complaints */
                int winner_idx;     /* Best channel */


                if (user_pref.winner_override_idx >= 0) {
                    winner_idx = user_pref.winner_override_idx;
                    rf_counters.manual++;
                    cause.reason = USER_OVERRIDE;
                    update_channel_log(channels[g_current_chan_idx], &cause);
                } else {
                    update_channel_log(channels[g_current_chan_idx], &cause);
                    winner_idx = pick_best(channels, g_scores, g_current_chan_idx);
                }

                if (winner_idx < 0) {
                    WPRINT_APP_INFO(("%s: Invalid best channel!!!!!\n", __FUNCTION__));
                    continue;
                }

                /* Clear pending switch requests. */
                memset(&cause, 0, sizeof(csa_cause_t));
                just_switched++;

                if (g_have_audio)
                    WPRINT_APP_INFO(("%s %3.2d[%d%%] => %3.2d[%d%% -> %d%%]\n",
                        get_formatted_time(&time_now, time_buf),
                        channels[g_current_chan_idx], PERCENT(g_scores[g_current_chan_idx]),
                        channels[winner_idx], PERCENT(g_scores[winner_idx]),
                        PERCENT(g_scores[winner_idx]+g_aud_format.score)));
                else
                    WPRINT_APP_INFO(("%s %3.2d[CCA %d%%] => %3.2d[CCA %d%%]\n",
                        get_formatted_time(&time_now, time_buf),
                        channels[g_current_chan_idx], PERCENT(g_scores[g_current_chan_idx]),
                        channels[winner_idx], PERCENT(g_scores[winner_idx])));

                /* Theory was to wait for the previous RM which possibly hasn't finished yet.
                 * Not sure I need this */
                wiced_rtos_delay_milliseconds(50);

                rf_counters.num_csas++;
                if (send_csa(winner_idx) != WWD_SUCCESS) {
                    WPRINT_APP_INFO(("%s: FAIL to send CSA to channel %d. Abort change.\n",
                        APP_NAME, channels[winner_idx]));

                }
                /* update time */
                time_last = time_now;
                chan_info[g_current_chan_idx].blacklist = time_now;

                g_current_chan_idx = winner_idx;
                user_pref.winner_override_idx = -1;

                /* Dont do any more until we actually do the switch */
                wiced_rtos_delay_milliseconds((NUM_CSA_BCN + 2) * 100);

                /* On a new channel, flush any report packets pending/queued from previous channel */
                sniff_report(&cause, FLUSH_REPORTS);
                clear_all_report_stats();
            }
        }

#ifdef INCLUDE_DISPLAY
        draw_scores(&u8g, channels, g_scores);
#endif
    }
}

static int
wl_format_ssid(char* ssid_buf, uint8_t *ssid, int ssid_len)
{
    int i, c;
    char *p = ssid_buf;

    if (ssid_len > 32) ssid_len = 32;

    for (i = 0; i < ssid_len; i++) {
        c = (int)ssid[i];
        if (c == '\\') {
            *p++ = '\\';
            *p++ = '\\';
        } else {
            *p++ = (char)c;
        }
    }
    *p = '\0';

    return p - ssid_buf;
}

/* Get current channel and SSID */
static wiced_result_t
get_bss_info()
{
    wiced_buffer_t buffer;
    wiced_buffer_t response;
    wiced_result_t result;
    wl_bss_info_t *bi;
    char bigbuf[BSS_BUFLEN]; /* Hold GET_BSS_INFO */

    sprintf(bss_ssid, "%s", "Not Assoc");

    if (wwd_sdpcm_get_ioctl_buffer( &buffer, BSS_BUFLEN) == NULL) {
        WPRINT_APP_INFO(("%s: Unable to malloc WLC_GET_BSS_INFO buffer\n", __FUNCTION__));
        return -1;
    }
    result = wwd_sdpcm_send_ioctl( SDPCM_GET, WLC_GET_BSS_INFO, buffer, &response, WWD_STA_INTERFACE );
    if ( result != WICED_SUCCESS ) {
        WPRINT_APP_INFO(("%s: WLC_GET_BSS_INFO Failed\n", __FUNCTION__));
        return result;
    }
    memcpy(bigbuf, host_buffer_get_current_piece_data_pointer( response ), BSS_BUFLEN);
    bi = (wl_bss_info_t*) (bigbuf + 4);

    if (dtoh32(bi->version) == WL_BSS_INFO_VERSION ) {
        wl_format_ssid(bss_ssid, bi->SSID, bi->SSID_len);
        bss_chan_idx = channel_to_idx(CHSPEC_CHANNEL(bi->chanspec));
    } else {
        WPRINT_APP_INFO(("%s: Unexpected WL_BSS_VERSION 0x%x\n", __FUNCTION__, (unsigned int)dtoh32(bi->version)));
    }
    host_buffer_release( response, WWD_NETWORK_RX );
    return result;
}

/* Issue Radio Measurement ivoars rm_req/rm_rep for given channel */
/* Return various error codes if failure */
static int
get_channel_score(int channel, int *score)
{
    wiced_buffer_t req_buffer;
    wiced_buffer_t rep_buffer;
    wiced_buffer_t response;
    wl_rm_req_t *rm_req;
    wl_rm_rep_t rm_rep;
    uint32_t* rep_data;
    int err;

    if (channel >= 52 && channel <= 144) {
        *score = 254;
        return WWD_SUCCESS;
    }
    /*
     * Send Request
     */
    rm_req = (wl_rm_req_t *)wwd_sdpcm_get_iovar_buffer(&req_buffer, sizeof(wl_rm_req_t), IOVAR_STR_RM_REQ);
    if (rm_req == NULL) {
        WPRINT_APP_INFO(("%s:get rm_req buffer failed\n", __FUNCTION__));
        return -1;
    }
    memset(rm_req, 0, sizeof(wl_rm_req_t));

    rm_req->count = 1;
    rm_req->req[0].type = WL_RM_TYPE_CCA;
    rm_req->req[0].dur = DURATION;
    rm_req->req[0].chanspec = CH20MHZ_CHSPEC(channel);

    err = wwd_sdpcm_send_iovar(SDPCM_SET, req_buffer, &response, WWD_STA_INTERFACE);
    if (err != WWD_SUCCESS) {
        WPRINT_APP_INFO(("%s: send_iovar req_buf FAIL %d\n", __FUNCTION__, err));
        return -2;
    }
    host_buffer_release(response, WWD_NETWORK_RX);

    /*
     * Let the request gather the data
     */
    wiced_rtos_delay_milliseconds(DURATION+10);

    /*
     * Get Report
     */
    rep_data = (uint32_t*)wwd_sdpcm_get_iovar_buffer(&rep_buffer, sizeof(wl_rm_rep_t), IOVAR_STR_RM_REP);
    if (rep_data == NULL) {
        return -3;
    }
    memset(&rm_rep, 0, sizeof(wl_rm_rep_t));

    if (wwd_sdpcm_send_iovar(SDPCM_GET, rep_buffer, &response, WWD_STA_INTERFACE) != WWD_SUCCESS)
    {
        return -4;
    }
    memcpy(&rm_rep, host_buffer_get_current_piece_data_pointer(response), sizeof(wl_rm_rep_t));
    host_buffer_release(response, WWD_NETWORK_RX);

    if (rm_rep.rep[0].flags == 4) {
        /* INCAPABLE */
        return -6;
    }

    if (((rm_rep.rep[0].chanspec & 0xff) == channel) &&
        (rm_rep.rep[0].dur == DURATION) &&
        (rm_rep.rep[0].flags == 0) &&
        (rm_rep.token == 0) &&
        (rm_rep.len == 28)) {
        *score = rm_rep.rep[0].data[0];

    } else {
        uint32_t *foo = (uint32_t *)&rm_rep;
        if (*foo == APOLLO_REPORT_MAGIC_TAG) {
            WPRINT_APP_INFO(("%s: Wow, Got REPORT_MAGIC!\n", __FUNCTION__));
        } else  {
            char *x = (char *)foo;
            WPRINT_APP_INFO(("%s: Unparsable rm report %x %c%c%c%c %c%c\n", __FUNCTION__, (unsigned int)*foo,
                *x, *(x+1), *(x+2), *(x+3), *(x+4), *(x+5)));
        }
        return -5;
    }
    return WWD_SUCCESS;
}

/* Return index of best channel. Factor in DELTA. */
static int
pick_best(const int *channels, int *scores, int cur_chan_idx)
{
        int i;
        int best_index = INVALID_IDX;
        int best_score = 300;
        int total_score = 0;
        wiced_time_t time_now;

        wiced_time_get_time(&time_now);
        congest_scores.worst = -1;

        while (best_index == INVALID_IDX) {
            for (i = 0; i < NUM_CHANNELS; i++) {
                /* Search for absolute lowest except current chan */
                if (scores[i] >= 0 && (scores[i] < best_score) && (i != cur_chan_idx)) {
                    if (!chan_info[i].blacklist || time_now - chan_info[i].blacklist > BLACKLIST_TIME) {
                        best_score = scores[i];
                        best_index = i;
                    }
                }
                /* Search for highest (worst) */
                if (scores[i] >= 0 && ((congest_scores.worst < 0) || (scores[i] > scores[congest_scores.worst]))) {
                    congest_scores.worst = i;
                }
                /* Calculate avg */
                if (scores[i] >= 0) {
                    total_score += scores[i];  /* Calc average */
                }
            }
            if (best_index == INVALID_IDX) {
                /* Clear the blacklist */
                int j;
                WPRINT_APP_INFO(("%s: No chans available, clear the blacklist\n", __FUNCTION__));
                for (j = 0; j < NUM_CHANNELS; j++) {
                    chan_info[j].blacklist = 0;
                }
            }
        }
        g_best_index = best_index;
        g_best_score = scores[best_index];

        if (cur_chan_idx == INVALID_IDX) {
            WPRINT_APP_INFO(("%s: WHOA, SHOULD NOT COME HERE\n", __FUNCTION__));
            return channel_to_idx(44);
        }

        return best_index;
}

static wwd_result_t
send_csa(int chan_idx)
{
    wwd_result_t err;
    wiced_chan_switch_t cs;
    memset(&cs, 0, sizeof(wiced_chan_switch_t));
    cs.chspec = CH20MHZ_CHSPEC(channels[chan_idx]);
    cs.count  = NUM_CSA_BCN;
    cs.mode   = 0;

    if ((err = wwd_wifi_send_csa(&cs, WWD_STA_INTERFACE)) != WWD_SUCCESS) {
        WPRINT_APP_INFO(("wwd_wifi_send_csa failed\n"));
    }
    return err;
}

/* Keep a LOG of previous channels, might be useful for debugging */
static void
update_channel_log(int channel, csa_cause_t *cause)
{
    history[log_index].reason = cause->reason;
    history[log_index].channel = channel;
    history[log_index].speaker_channel = cause->speaker_channel;
    memcpy(history[log_index].mac_addr, cause->mac_addr, MACADDR_SZ);
    wiced_time_get_time(&history[log_index].time);
    log_index = (log_index + 1) % LOG_SIZE;
}


static char *
get_formatted_time(wiced_time_t *time, char *buf)
{
    int secs;

    secs = (*time - 0)/1000;
    sprintf(buf, "%d:%2.2d:%2.2d ", secs/(60*60), (secs/60)%60, secs % 60);
    return buf;
}

/* Dump the channel history */
static void
dump_channel_log()
{
    int idx = log_index;
    int count = 0;
    int secs;

    WPRINT_APP_INFO(("Channel switch history:\n"));
    WPRINT_APP_INFO(("\tLeft\n"));
    WPRINT_APP_INFO(("Time\tChannel\t   MAC Addr\t     Speaker  Reason\n"));
    for (count = 0; count < LOG_SIZE; count++) {
        if (history[idx].channel) {
            char sm_buf[16];
            secs = (history[idx].time - 0)/1000;
            WPRINT_APP_INFO(("%d:%2.2d:%2.2d\t", secs/(60*60), (secs/60)%60, secs % 60));
            WPRINT_APP_INFO(("  %d \t", history[idx].channel));
            WPRINT_APP_INFO((""MACDBG"\t", MAC2STRDBG(history[idx].mac_addr)));
            WPRINT_APP_INFO(("%s    ", speaker_to_name(history[idx].speaker_channel, sm_buf, sizeof(sm_buf))));
            switch (history[idx].reason) {
            case OVER_INSTANT_AUD_THRESH:
                WPRINT_APP_INFO(("Momentary AUD packet loss\n"));
                break;
            case OVER_MOVING_AUD_THRESH:
                WPRINT_APP_INFO(("Moving Avg AUD packet loss\n"));
                break;
            case OVER_ALL_AUD_THRESH:
                WPRINT_APP_INFO(("Avg of Moving AUD Avg across all Sinks\n"));
                break;
            case OVER_INSTANT_RTP_THRESH:
                WPRINT_APP_INFO(("Momentary RTP packet loss\n"));
                break;
            case OVER_MOVING_RTP_THRESH:
                WPRINT_APP_INFO(("Moving Avg RTP packet loss\n"));
                break;
            case OVER_ALL_RTP_THRESH:
                WPRINT_APP_INFO(("Avg of Moving RTP Avg across all Sinks\n"));
                break;
            case USER_OVERRIDE:
                WPRINT_APP_INFO(("User command line\n"));
                break;
            default:
                WPRINT_APP_INFO(("\n"));
                break;
            }
        }
        idx = (idx + 1) % LOG_SIZE;
    }
    WPRINT_APP_INFO(("\n"));
}

#ifdef INCLUDE_DISPLAY
static void u8g_prepare(u8g_t* u8g) {
    u8g_SetFont(u8g, u8g_font_6x10);
    u8g_SetFontRefHeightExtendedText(u8g);
    u8g_SetDefaultForegroundColor(u8g);
    u8g_SetFontPosTop(u8g);
}

static void
draw_scores(u8g_t *u8g, const int *channels, int *scores)
{
    u8g_FirstPage(u8g);
    do {
        make_page(u8g, channels, scores);
    } while (u8g_NextPage(u8g));
}

/* u8g_box_frame  */
/*******
  0,0 +---------+ 128,0
      |         |
      |         |
 0,63 +---------+ 128,63
***********/

static void
make_page(u8g_t *u8g, const int *channels, int *scores)
{
    char buf[32];
    uint8_t h;
    int i;

    /*
     * Print header: Current channel, time on channel, Avg Load
     */
    if (bss_chan_idx != -1) {
        sprintf(buf, "%d[%d%%] %s",
            channels[bss_chan_idx], PERCENT(scores[bss_chan_idx]), bss_ssid);
    } else {
        /* Indicates we couldn't get bss info and we are not assoc'ed */
        sprintf(buf, "-- %s", bss_ssid);
    }
    u8g_prepare(u8g);
    u8g_DrawStr(u8g, 0, 0, buf);

    /*
     * Draw bars
     */
    for (i = 0; i < NUM_CHANNELS; i++) {
        h = MAP_BAR_HEIGHT(scores[i]);
        if (h == 0xff)
            continue;
        h = MIN(h, BAR_BASE - HEADER_HEIGHT);
        if (h) {
            /* Attempting to draw 0 height causes problems */
            u8g_DrawBox(u8g, i * (BAR_WIDTH+BAR_GAP), BAR_BASE - h, BAR_WIDTH, h);
        }
        /* If score has decreased since last run, show the previous score */
        if (prev_scores[i] > scores[i]) {
            int prev_h = MAP_BAR_HEIGHT(prev_scores[i]);
            prev_h = MIN(prev_h, BAR_BASE - HEADER_HEIGHT);
            u8g_DrawHLine(u8g, i * (BAR_WIDTH+BAR_GAP), BAR_BASE - prev_h, BAR_WIDTH);
        }
    }

    /*
     * Draw Labels on bottom
     */
    u8g_SetFont(u8g, u8g_font_4x6);
    u8g_SetFontRefHeightExtendedText(u8g);
    u8g_SetDefaultForegroundColor(u8g);
    u8g_SetFontPosTop(u8g);
    for (i = 0; i < NUM_CHANNELS; i++) {
        sprintf(buf, "%d", channels[i]);
        u8g_DrawStr270(u8g, i * (BAR_WIDTH+BAR_GAP) - 1, BOTTOM, buf);
    }
}
#endif

static int
channel_to_idx(int channel)
{
    int i;
    for (i = 0; i < NUM_CHANNELS; i++) {
        if (channel == channels[i]) {
            return i;
        }
    }
    return -1;
}

static void
dump_blacklist()
{
    int i;
    wiced_time_t time_now;
    wiced_time_get_time(&time_now);
    WPRINT_APP_INFO(("%d second Blacklist:\n", BLACKLIST_TIME/1000));
    WPRINT_APP_INFO(("Channel\t\tSeconds to Expire\n"));
    for (i = 0; i < NUM_CHANNELS; i++) {
        if (chan_info[i].blacklist && (time_now - chan_info[i].blacklist < BLACKLIST_TIME)) {
            WPRINT_APP_INFO(("%d\t\t%ld\n",  channels[i], (BLACKLIST_TIME - (time_now - chan_info[i].blacklist))/1000));
        }
    }
}

int
rfmon_console_command(int argc, char *argv[])
{
    int i;
    int tmp;
    for (i = 0; i < RFMON_CONSOLE_CMD_MAX; ++i)
    {
        if (strcmp(command_lookup[i].cmd, argv[0]) == 0)
            break;
    }

    if (i >= RFMON_CONSOLE_CMD_MAX)
    {
        WPRINT_APP_INFO(("%s: Unrecognized command: %s\n", __FUNCTION__, argv[0]));
        return ERR_CMD_OK;
    }

    switch (i)
    {
        case RFMON_CONSOLE_CMD_CSA:
            tmp = atoi(argv[1]);
            tmp = channel_to_idx(tmp);
            if (tmp < 0) {
                WPRINT_APP_INFO(("%s: Unknown channel: %s, ignoring\n", __FUNCTION__, argv[1]));
                break;
            }
            /* Set manual override */
            if (user_pref.winner_override_idx < 0)
                user_pref.winner_override_idx = tmp;
            break;
        case RFMON_CONSOLE_CMD_ON:
            WPRINT_APP_INFO(("%s: Enabling CSA's\n", __FUNCTION__));
            user_pref.skip_csa = 0;
            break;
        case RFMON_CONSOLE_CMD_OFF:
            user_pref.skip_csa = 1;
            WPRINT_APP_INFO(("%s: Disabling CSA's\n", __FUNCTION__));
            break;
        case RFMON_CONSOLE_LOGGING:
            if (argc >= 2) {
                user_pref.report_logging = atoi(argv[1]);
            } else {
                WPRINT_APP_INFO(("Logging = %d\n", user_pref.report_logging));
            }
            break;
        case RFMON_CONSOLE_BLACKLIST:
            dump_blacklist();
            break;
        case RFMON_CONSOLE_THRESHOLD:
            if (argc < 3) {
                WPRINT_APP_INFO(("[1] Momentary AUD packet loss %% on a Sink: \t%d%%\n", rf_thresh.instant_aud_thresh));
                WPRINT_APP_INFO(("[2] Moving Avg AUD packet loss on a Sink %%: \t%d%%\n",    rf_thresh.moving_aud_thresh));
                WPRINT_APP_INFO(("[3] Avg of Moving AUD Avg across all Sink %%: \t%d%%\n",  rf_thresh.all_aud_thresh));
                WPRINT_APP_INFO(("\n"));
                WPRINT_APP_INFO(("[4] Instantaeous RTP packet loss %% on a Sink: \t%d%%\n",  rf_thresh.instant_rtp_thresh));
                WPRINT_APP_INFO(("[5] Moving Avg RTP packet loss on a Sink %%: \t%d%%\n",    rf_thresh.moving_rtp_thresh));
                WPRINT_APP_INFO(("[6] Avg of Moving RTP Avg across all Sink %%: \t%d%%\n",  rf_thresh.all_rtp_thresh));
                WPRINT_APP_INFO(("\n"));
                WPRINT_APP_INFO(("To SET a new threshold, enter tag number from above and value.\n"));
                WPRINT_APP_INFO(("For example: rf_thresh 4 10, sets instantaneous RTP to 10%%.\n"));
                WPRINT_APP_INFO(("\n"));
                WPRINT_APP_INFO(("Minimum Seconds between channel switches: \t%d\n", MIN_SECS));
                WPRINT_APP_INFO(("Minimum Reports between channel switches: \t%d\n", MIN_REPORTS));
            } else {
                int tmp = atoi(argv[2]);
                switch(atoi(argv[1])) {
                case 1:
                    rf_thresh.instant_aud_thresh = tmp;
                    break;
                case 2:
                    rf_thresh.moving_aud_thresh = tmp;
                    break;
                case 3:
                    rf_thresh.all_aud_thresh = tmp;
                    break;
                case 4:
                    rf_thresh.instant_rtp_thresh = tmp;
                    break;
                case 5:
                    rf_thresh.moving_rtp_thresh = tmp;
                    break;
                case 6:
                    rf_thresh.all_rtp_thresh = tmp;
                    break;
                }
            }
            break;
        case RFMON_CONSOLE_CMD_STATUS:
        {
            wiced_time_t time_now;
            char time_buf[16];
            wiced_time_get_time(&time_now);
            WPRINT_APP_INFO(("Time Since Boot: %s\n", get_formatted_time(&time_now, time_buf)));
            WPRINT_APP_INFO(("CSA status:  %s\n", user_pref.skip_csa ? "Disabled" : "Enabled"));
            WPRINT_APP_INFO(("Current channel: %3.0d, Congestion Score %d%%\n", channels[g_current_chan_idx], PERCENT(g_current_score)));

            WPRINT_APP_INFO(("CSAs since boot:  %d\n", rf_counters.num_csas));
            WPRINT_APP_INFO(("Theshold violations.  All violations are logged but only first triggers CSA:\n"));
            WPRINT_APP_INFO(("\tFrom command line:   \t%d\n" , rf_counters.manual));
            WPRINT_APP_INFO(("\tMoving Aud Thresh[%d%%]:\t%d\n", rf_thresh.moving_aud_thresh,  rf_counters.moving_aud_thresh));
            WPRINT_APP_INFO(("\tMoving RTP Thresh[%d%%]:\t%d\n", rf_thresh.moving_rtp_thresh,  rf_counters.moving_rtp_thresh));
            WPRINT_APP_INFO(("\tSingle Aud Thresh[%d%%]:\t%d\n", rf_thresh.instant_aud_thresh, rf_counters.instantaneous_aud_thresh));
            WPRINT_APP_INFO(("\tSingle RTP Thresh[%d%%]:\t%d\n", rf_thresh.instant_rtp_thresh, rf_counters.instantaneous_rtp_thresh));
            WPRINT_APP_INFO(("\tAll Aud Thresh[%d%%]:\t%d\n", rf_thresh.all_aud_thresh,     rf_counters.all_aud_thresh));
            WPRINT_APP_INFO(("\tAll RTP Thresh[%d%%]:\t%d\n", rf_thresh.all_rtp_thresh,     rf_counters.all_rtp_thresh));
            //WPRINT_APP_INFO(("Reports Flushed:       \t%d\n" , rf_counters.too_soon));
            WPRINT_APP_INFO(("\n"));

            dump_blacklist();
            WPRINT_APP_INFO(("\n"));

            dump_channel_log();
            WPRINT_APP_INFO(("\n"));

            WPRINT_APP_INFO(("                    Last/Avg   Last/Avg   Last/Avg   Speaker  Reports Since\n"));
            WPRINT_APP_INFO(("   MAC Address        RSSI     RTP Drop  Audio Drop  Channel  Last CSA\n"));
            {
                int i;
                apollo_stats_t *stats;
                char sm_buf[16];
                char empty[MACADDR_SZ] = {0};
                for (i = 0; i < MAX_DEVICES; i++) {
                    stats = &apollo_stats[i];
                    if (memcmp(stats->mac_addr, empty, MACADDR_SZ) != 0) {
                        WPRINT_APP_INFO((""MACDBG"    %d/%d     %d%%/%d%%     %d%%/%d%%     \t%s\t%d",
                            MAC2STRDBG(stats->mac_addr),
                            stats->last_rssi, stats->rssi_ma,
                            stats->last_rtp_drop_rate, stats->rtp_ma,
                            stats->last_aud_drop_rate, stats->aud_ma,
                            speaker_to_name(stats->speaker_channel, sm_buf, sizeof(sm_buf)),
                            stats->num_rpts_rx));
                        if ((time_now - stats->time) > (1000 * LOST_SECS))
                            WPRINT_APP_INFO(("  Last Heard %ld Secs ago", (time_now - stats->time)/1000));
                        WPRINT_APP_INFO(("\n"));
                    }
                }
            }
            }
            break;
        case RFMON_CONSOLE_CMD_HISTORY:
            dump_channel_log();
            break;
    }
    return 0;
}

static apollo_stats_t *
mac2stats(uint8_t *mac)
{
    int i;
    for (i = 0; i < MAX_DEVICES; i++) {
        if (memcmp(mac, apollo_stats[i].mac_addr, MACADDR_SZ) == 0) {
            return &apollo_stats[i];
        }
    }
    return NULL;
}

/* Clear the struct but save/restore a few fields */
static void
clear_report_stats(apollo_stats_t *stats)
{
    uint8_t tmp_mac[MACADDR_SZ];
    wiced_time_t tmp_time;
    int tmp_speaker;

    /* These are the actual counters from packets...preserve
     * so we maintain counters over chan switch.
     * All the others are derived from these, blow them away and
     * start fresh */
    /*
    rtp_packets_received;
    rtp_packets_dropped;
    audio_frames_played;
    audio_frames_dropped;
    **/

    /* Save these */
    memcpy(tmp_mac, stats->mac_addr, MACADDR_SZ);
    tmp_time = stats->time;
    tmp_speaker = stats->speaker_channel;

    /* Clear */
    memset(stats, 0, sizeof(apollo_stats_t));

    /* Restore */
    memcpy(stats->mac_addr, tmp_mac, MACADDR_SZ);
    stats->time = tmp_time;
    stats->speaker_channel = tmp_speaker;
}

static void
clear_all_report_stats()
{
    int i;
    for (i = 0; i < MAX_DEVICES; i++) {
        clear_report_stats(&apollo_stats[i]);
    }
}

static apollo_stats_t *
alloc_stats(uint8_t *mac)
{
    int i;
    char empty[MACADDR_SZ] = {0};
    for (i = 0; i < MAX_DEVICES; i++) {
        if (memcmp(apollo_stats[i].mac_addr, empty, MACADDR_SZ) == 0) {
            clear_report_stats(&apollo_stats[i]);
            memcpy(apollo_stats[i].mac_addr, mac, MACADDR_SZ);
            return &apollo_stats[i];
        }
    }
    return NULL;
}

/* Calculate Moving Average for all devices */
static void
calc_all_ma(csa_cause_t *cause)
{
    int i, n_samples = 0;
    int tot_aud = 0, tot_rtp = 0, tot_rssi = 0;
    for (i = 0; i < MAX_DEVICES; i++) {
        if (apollo_stats[i].rtp_packets_received != 0) {
            tot_aud += apollo_stats[i].aud_ma;
            tot_rtp += apollo_stats[i].rtp_ma;
            tot_rssi += apollo_stats[i].rssi_ma;
            n_samples++;
        }
    }
    if (n_samples) {
        g_all_avg_rtp = tot_rtp/n_samples;
        g_all_avg_aud = tot_aud/n_samples;
        g_all_avg_rssi = tot_rssi/n_samples;
    }

    if (g_all_avg_rtp >= rf_thresh.all_rtp_thresh) {
        rf_counters.all_rtp_thresh++;
        if (!cause->reason) {
            cause->reason = OVER_ALL_RTP_THRESH;
            WPRINT_APP_INFO(("Avg RTP for all devices[%d%%] >= thresh[%d%%]\n",
                g_all_avg_rtp, rf_thresh.all_rtp_thresh));
        }
    }
    if (g_all_avg_aud >= rf_thresh.all_aud_thresh) {
        rf_counters.all_aud_thresh++;
        if (!cause->reason) {
            cause->reason = OVER_ALL_AUD_THRESH;
            WPRINT_APP_INFO(("Avg Audio for all devices[%d%%] >= thresh[%d%%]\n",
                g_all_avg_aud, rf_thresh.all_aud_thresh));
        }
    }
}

/* Calculate Moving Average for this device */
static void
calc_ma(csa_cause_t *cause, apollo_stats_t *stats)
{
    int i, samps, total;

    /* Audio */
    total = samps = 0;
    for (i = 0; i < MA_WIN_SZ; i++) {
        if (stats->aud_drop_rate_buf[i] != INVALID_PERCENT) {
            total += stats->aud_drop_rate_buf[i];
            samps++;
        }
    }
    if (samps)
        stats->aud_ma = total/samps;

    /* RTP */
    total = samps = 0;
    for (i = 0; i < MA_WIN_SZ; i++) {
        if (stats->rtp_drop_rate_buf[i] != INVALID_PERCENT) {
            total += stats->rtp_drop_rate_buf[i];
            samps++;
        }
    }
    if (samps)
        stats->rtp_ma = total/samps;

    /* RSSI */
    total = samps = 0;
    for (i = 0; i < MA_WIN_SZ; i++) {
        if (stats->rssi_buf[i] != INVALID_PERCENT) {
            total += stats->rssi_buf[i];
            samps++;
        }
    }
    if (samps)
        stats->rssi_ma = total/samps;

    if (stats->aud_ma >= rf_thresh.moving_aud_thresh) {
        rf_counters.moving_aud_thresh++;
        if (!cause->reason) {
            cause->reason = OVER_MOVING_AUD_THRESH;
            WPRINT_APP_INFO(("Avg Audio for "MACDBG"[%d%%] >= thresh[%d%%]\n",
                MAC2STRDBG(stats->mac_addr), stats->aud_ma, rf_thresh.moving_aud_thresh));
        }
    }
    if (stats->rtp_ma >= rf_thresh.moving_rtp_thresh) {
        rf_counters.moving_rtp_thresh++;
        if (!cause->reason) {
            WPRINT_APP_INFO(("Avg RTP for "MACDBG"[%d%%] >= thresh[%d%%]\n",
                MAC2STRDBG(stats->mac_addr), stats->rtp_ma, rf_thresh.moving_rtp_thresh));
            cause->reason = OVER_MOVING_RTP_THRESH;
        }
    }
}

static void
dump_ma(apollo_stats_t *stats)
{
    int i, ma = (stats->ma_idx + 1) % MA_WIN_SZ;
    int samps = 0, total = 0;
    WPRINT_APP_INFO(("Aud: "));
    for (i = 0; i < MA_WIN_SZ; i++) {
        if (stats->aud_drop_rate_buf[ma] != INVALID_PERCENT) {
            WPRINT_APP_INFO(("%d ", stats->aud_drop_rate_buf[ma]));
            total += stats->aud_drop_rate_buf[ma];
            samps++;
        }
        ma  = (ma + 1) % MA_WIN_SZ;
    }
    if (samps)
        WPRINT_APP_INFO(("\tAvg[%d] %d\n", samps, total/samps));

    total = samps = 0;
    WPRINT_APP_INFO(("RTP: "));
    for (i = 0; i < MA_WIN_SZ; i++) {
        if (stats->rtp_drop_rate_buf[ma] != INVALID_PERCENT) {
            WPRINT_APP_INFO(("%d ", stats->rtp_drop_rate_buf[ma]));
            total += stats->rtp_drop_rate_buf[ma];
            samps++;
        }
        ma = (ma + 1) % MA_WIN_SZ;
    }
    if (samps)
        WPRINT_APP_INFO(("\tAvg[%d] %d\n", samps, total/samps));
}

static void
process_stats(csa_cause_t *cause, apollo_report_stats_t *incoming, uint8_t *macaddr)
{
    apollo_stats_t *stats;
    int delta_rtp_drop, delta_rtp_rx, delta_aud_drop, delta_aud_played;
    int rtp_drop_rate, aud_drop_rate;
    wiced_time_t time_now;

    if (cause->reason) {
        WPRINT_APP_INFO(("Entering process_stats: cause already set! - return\n"));
        return;
    }
    if (incoming->version != APOLLO_REPORT_STATS_VERSION) {
        WPRINT_APP_INFO(("%s: Bad version: %x\n", __FUNCTION__, incoming->version));
        return;
    }
    if ((stats = mac2stats(macaddr)) == NULL) {
        char sm_buf[16];
        if ((stats = alloc_stats(macaddr)) == NULL) {
            WPRINT_APP_INFO(("%s: Can't allocate stats buffer\n", __FUNCTION__));
            return;
        }
        stats->speaker_channel =  incoming->speaker_channel;
        WPRINT_APP_INFO(("Adding "MACDBG" %s\n",
            MAC2STRDBG(macaddr),
            speaker_to_name(stats->speaker_channel, sm_buf, sizeof(sm_buf))));
    }

    /* Populate these even prior to MIN_REPORTS */
    wiced_time_get_time(&time_now);
    stats->time = time_now;
    stats->num_rpts_rx++;
    stats->last_rssi = incoming->rssi;
    stats->rssi_buf[stats->ma_idx] = incoming->rssi;

    /* Drop the first MIN_REPORTS, they may be from previous channel */
    if (stats->num_rpts_rx > MIN_REPORTS) {
        /* Deltas from previous packet */
        delta_rtp_drop =   incoming->rtp_packets_dropped  - stats->rtp_packets_dropped;
        delta_rtp_rx =     incoming->rtp_packets_received - stats->rtp_packets_received;
        delta_aud_drop =   incoming->audio_frames_dropped - stats->audio_frames_dropped;
        delta_aud_played = incoming->audio_frames_played  - stats->audio_frames_played;

        stats->rtp_packets_dropped =  incoming->rtp_packets_dropped  ;
        stats->rtp_packets_received = incoming->rtp_packets_received ;
        stats->audio_frames_dropped = incoming->audio_frames_dropped ;
        stats->audio_frames_played =  incoming->audio_frames_played  ;

        /* Negative counts indicate Sink was rebooted (and restarted counts) */
        if ((delta_rtp_rx < 0) && (delta_aud_played < 0)) {
            WPRINT_APP_INFO((""MACDBG" was rebooted.\n", MAC2STRDBG(macaddr)));
            return;
        }

        /* Percentages/Rates for RTP and Audio */
        rtp_drop_rate = delta_rtp_rx ? (delta_rtp_drop*100)/delta_rtp_rx : 0;
        aud_drop_rate = delta_aud_played ? (delta_aud_drop*100)/delta_aud_played : 0;

        /* Add these latest rates to moving average buffer */
        stats->rtp_drop_rate_buf[stats->ma_idx] = rtp_drop_rate;
        stats->aud_drop_rate_buf[stats->ma_idx] = aud_drop_rate;

        /* Also keep it seperate for easy dumping */
        stats->last_rtp_drop_rate = rtp_drop_rate;
        stats->last_aud_drop_rate = aud_drop_rate;

        /* Keep the first reason but check all conditions so they can all be logged */
        if (stats->last_rtp_drop_rate >= rf_thresh.instant_rtp_thresh) {
            rf_counters.instantaneous_rtp_thresh++;
            if (!cause->reason) {
                WPRINT_APP_INFO(("Momentary RPT for "MACDBG"[%d%%] >= thresh[%d%%], num_rpts %d\n",
                    MAC2STRDBG(stats->mac_addr), stats->last_rtp_drop_rate, rf_thresh.instant_rtp_thresh,
                    stats->num_rpts_rx));
                cause->reason = OVER_INSTANT_RTP_THRESH;
            }
        }
        if (stats->last_aud_drop_rate >= rf_thresh.instant_aud_thresh) {
            rf_counters.instantaneous_aud_thresh++;
            if (!cause->reason) {
                WPRINT_APP_INFO(("Momentary AUD for "MACDBG"[%d%%] >= thresh[%d%%]\n",
                    MAC2STRDBG(stats->mac_addr), stats->last_aud_drop_rate, rf_thresh.instant_aud_thresh));
                cause->reason = OVER_INSTANT_AUD_THRESH;
            }
        }

        /* Calculate averages for this specific device */
        calc_ma(cause, stats);

        if (cause->reason) {
            memcpy(cause->mac_addr, stats->mac_addr, MACADDR_SZ);
            cause->speaker_channel = stats->speaker_channel;
        }

        /* Calc avg for ALL devices, so a specific macaddr/speaker does not apply */
        calc_all_ma(cause);

        if (user_pref.report_logging)  {
        WPRINT_APP_INFO((""MACDBG": RSSI %d/%d   RTP %d/%d\t=%d%%/%d%%   Aud %d/%d\t=%d%%/%d%%  All: RTP %d%% AUD %d%%\n",
             MAC2STRDBG(macaddr), incoming->rssi, stats->rssi_ma,
             delta_rtp_drop, delta_rtp_rx,
             rtp_drop_rate, stats->rtp_ma,
             delta_aud_drop, delta_aud_played,
             aud_drop_rate, stats->aud_ma,
             g_all_avg_rtp, g_all_avg_aud
             ));
        }

        if (0)
            dump_ma(stats);

        /* Bump moving average index */
        stats->ma_idx = (stats->ma_idx + 1) % MA_WIN_SZ;

    } /* MIN_REPORTS */
    return;
}

static wiced_result_t
sniff_report(csa_cause_t *cause, int action)
{
    wiced_result_t result = WICED_SUCCESS;
    wiced_packet_t* report_packet;
    uint8_t* report_data = NULL;
    uint16_t data_length = 0;
    uint16_t available_data_length;
    apollo_report_msg_t *report;

    if (cause->reason && (action == PROCESS_REPORTS)) {
        WPRINT_APP_INFO(("Entering %s: cause->reason already set!\n", __FUNCTION__));
    }

    while ((result = wiced_udp_receive(&glob_stats_socket, &report_packet, WICED_NO_WAIT)) == WICED_SUCCESS) {
        result = wiced_packet_get_data(report_packet, 0, &report_data, &data_length, &available_data_length);
        if (result != WICED_SUCCESS || report_data == NULL || data_length == 0) {
            WPRINT_APP_INFO(("%s: Invalid Report packet at 0x%08lX, with size = %d\n",
                __FUNCTION__, (uint32_t)report_data, data_length));
            return WICED_ERROR;
        }

        if (data_length < sizeof(apollo_report_msg_t)) {
            WPRINT_APP_INFO(("%s: Report packet too small: %d\n", __FUNCTION__, data_length));
            return WICED_ERROR;
        }
        report = (apollo_report_msg_t *)report_data;
        if (report->magic != APOLLO_REPORT_MAGIC_TAG) {
            WPRINT_APP_INFO(("%s: MAGIC TAG fails: 0x%x\n", __FUNCTION__, (unsigned int)report->magic));
            return WICED_ERROR;
        }

        if (report->msg_type == APOLLO_REPORT_MSG_STATS) {
            if (action == PROCESS_REPORTS) {
                /*
                 *  We could break from this loop once a reason is found but that would require
                 *  we call routine again to flush remaining buffers anyway. Also would need to
                 *  duplicate the packet_delete().
                 *  Bypass process_stats() when we already have a reason otherwise that would overwrite
                 *  the original cause.
                 */
                if (!cause->reason) {
                    process_stats(cause, (apollo_report_stats_t *)report->msg_data, report->mac_addr);
                }
            } else {
                rf_counters.too_soon++;
            }
        }

        wiced_packet_delete(report_packet);
    }

    switch(result) {
    case WICED_TCPIP_TIMEOUT:
        break;
    case WICED_TCPIP_WAIT_ABORTED:
        WPRINT_APP_INFO(("%s: WAIT_ABORTED\n", __FUNCTION__));
        break;
    default:
        WPRINT_APP_INFO(("%s: Other\n", __FUNCTION__));
        break;
    }
    return WICED_SUCCESS;
}
