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

#ifdef __cplusplus
extern "C" {
#endif

wiced_result_t create_ping_worker_thread( void );

extern int sta_get_ip_config             ( int argc, char *argv[] );
extern int sta_set_ip_config             ( int argc, char *argv[] );
extern int sta_get_info                  ( int argc, char *argv[] );
extern int sta_get_mac_address           ( int argc, char *argv[] );
extern int sta_is_connected              ( int argc, char *argv[] );
extern int sta_verify_ip_connection      ( int argc, char *argv[] );
extern int sta_get_bssid                 ( int argc, char *argv[] );
extern int device_get_info               ( int argc, char *argv[] );
extern int device_list_interfaces        ( int argc, char *argv[] );
extern int sta_set_encryption            ( int argc, char *argv[] );
extern int sta_set_psk                   ( int argc, char *argv[] );
extern int sta_associate                 ( int argc, char *argv[] );
extern int sta_preset_testparameters     ( int argc, char *argv[] );
extern int traffic_send_ping             ( int argc, char *argv[] );
extern int traffic_stop_ping             ( int argc, char *argv[] );
extern int traffic_agent_config          ( int argc, char *argv[] );
extern int traffic_agent_reset           ( int argc, char *argv[] );
extern int traffic_agent_send            ( int argc, char *argv[] );
extern int traffic_agent_receive_start   ( int argc, char *argv[] );
extern int traffic_agent_receive_stop    ( int argc, char *argv[] );
extern int sta_set_11n                   ( int argc, char *argv[] );
extern int sta_disconnect                ( int argc, char *argv[] );
extern int sta_reassoc                   ( int argc, char *argv[] );
extern int sta_p2p_reset                 ( int argc, char *argv[] );
extern int sta_get_p2p_dev_address       ( int argc, char *argv[] );
extern int sta_set_p2p                   ( int argc, char *argv[] );
extern int sta_start_autonomous_go       ( int argc, char *argv[] );
extern int sta_get_p2p_ip_config         ( int argc, char *argv[] );
extern int sta_wps_read_pin              ( int argc, char *argv[] );
extern int sta_p2p_start_group_formation ( int argc, char *argv[] );
extern int sta_set_wps_pbc               ( int argc, char *argv[] );
extern int sta_wps_enter_pin             ( int argc, char *argv[] );
extern int sta_p2p_connect               ( int argc, char *argv[] );
extern int sta_get_psk                   ( int argc, char *argv[] );
extern int sta_set_sleep                 ( int argc, char *argv[] );
extern int sta_accept_p2p_invitation_req ( int argc, char *argv[] );
extern int sta_send_p2p_invitation_req   ( int argc, char *argv[] );
extern int sta_p2p_dissolve              ( int argc, char* argv[] );

#define WIFI_CERT_COMMANDS \
    { "sta_get_ip_config",               sta_get_ip_config,                         0,         NULL, NULL, "", "" }, \
    { "sta_set_ip_config",               sta_set_ip_config,                         0,         NULL, NULL, "", "" }, \
    { "sta_get_info",                    sta_get_info,                              0,         NULL, NULL, "", "" }, \
    { "sta_get_mac_address",             sta_get_mac_address,                       0,         NULL, NULL, "", "" }, \
    { "sta_is_connected",                sta_is_connected,                          0,         NULL, NULL, "", "" }, \
    { "sta_verify_ip_connection",        sta_verify_ip_connection,                  0,         NULL, NULL, "", "" }, \
    { "sta_get_bssid",                   sta_get_bssid,                             0,         NULL, NULL, "", "" }, \
    { "device_get_info",                 device_get_info,                           0,         NULL, NULL, "", "" }, \
    { "device_list_interfaces",          device_list_interfaces,                    0,         NULL, NULL, "", "" }, \
    { "sta_set_encryption",              sta_set_encryption,                        0,         NULL, NULL, "", "" }, \
    { "sta_set_psk",                     sta_set_psk,                               1,         NULL, NULL, "", "" }, \
    { "sta_associate",                   sta_associate,                             0,         NULL, NULL, "", "" }, \
    { "sta_preset_testparameters",       sta_preset_testparameters,                 0,         NULL, NULL, "", "" }, \
    { "traffic_send_ping",               traffic_send_ping,                         0,         NULL, NULL, "", "" }, \
    { "traffic_stop_ping",               traffic_stop_ping,                         0,         NULL, NULL, "", "" }, \
    { "traffic_agent_config",            traffic_agent_config,                      0,         NULL, NULL, "", "" }, \
    { "traffic_agent_reset",             traffic_agent_reset,                       0,         NULL, NULL, "", "" }, \
    { "traffic_agent_send",              traffic_agent_send,                        0,         NULL, NULL, "", "" }, \
    { "traffic_agent_receive_start",     traffic_agent_receive_start,               0,         NULL, NULL, "", "" }, \
    { "traffic_agent_receive_stop",      traffic_agent_receive_stop,                0,         NULL, NULL, "", "" }, \
    { "sta_set_11n",                     sta_set_11n,                               0,         NULL, NULL, "", "" }, \
    { "sta_disconnect",                  sta_disconnect,                            0,         NULL, NULL, "", "" }, \
    { "sta_reassoc",                     sta_reassoc,                               0,         NULL, NULL, "", "" },

#ifdef WICED_USE_WIFI_P2P_INTERFACE
#define WIFI_P2P_CERT_COMMANDS \
    { "sta_p2p_reset",                   sta_p2p_reset,                             0,         NULL, NULL, "", "" }, \
    { "sta_get_p2p_dev_address",         sta_get_p2p_dev_address,                   0,         NULL, NULL, "", "" }, \
    { "sta_set_p2p",                     sta_set_p2p,                               0,         NULL, NULL, "", "" }, \
    { "sta_start_autonomous_go",         sta_start_autonomous_go,                   0,         NULL, NULL, "", "" }, \
    { "sta_get_p2p_ip_config",           sta_get_p2p_ip_config,                     0,         NULL, NULL, "", "" }, \
    { "sta_wps_read_pin",                sta_wps_read_pin,                          0,         NULL, NULL, "", "" }, \
    { "sta_p2p_start_group_formation",   sta_p2p_start_group_formation,             0,         NULL, NULL, "", "" }, \
    { "sta_set_wps_pbc",                 sta_set_wps_pbc,                           0,         NULL, NULL, "", "" }, \
    { "sta_wps_enter_pin",               sta_wps_enter_pin,                         0,         NULL, NULL, "", "" }, \
    { "sta_p2p_connect",                 sta_p2p_connect,                           0,         NULL, NULL, "", "" }, \
    { "sta_get_psk",                     sta_get_psk,                               0,         NULL, NULL, "", "" }, \
    { "sta_set_sleep",                   sta_set_sleep,                             0,         NULL, NULL, "", "" }, \
    { "sta_accept_p2p_invitation_req",   sta_accept_p2p_invitation_req,             0,         NULL, NULL, "", "" }, \
    { "sta_send_p2p_invitation_req",     sta_send_p2p_invitation_req,               0,         NULL, NULL, "", "" }, \
    { "sta_p2p_dissolve",                sta_p2p_dissolve,                          0,         NULL, NULL, "", "" },
#else
#define WIFI_P2P_CERT_COMMANDS
#endif


#ifdef __cplusplus
} /* extern "C" */
#endif
