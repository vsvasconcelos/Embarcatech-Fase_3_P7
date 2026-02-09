/**
 * @file ble_server.c
 * @brief BLE GATT Server implementation for Robot Control
 */

#include "ble_server.h"
#include "config.h"
#include "robo.h"  /* Generated from robo.gatt */

#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

#include "btstack.h"

/* ==========================================================================
 * Constants
 * ========================================================================== */

#define ROBOT_NAME "RoboMovel"
#define MAX_STATUS_LEN 32

/* Advertising interval: 100ms - 200ms */
#define ADV_INTERVAL_MIN 0x00A0  /* 100ms */
#define ADV_INTERVAL_MAX 0x0140  /* 200ms */

/* ==========================================================================
 * Static Variables
 * ========================================================================== */

/* Robot state */
static RobotState g_robot_state = {
    .running = false,
    .color_mode = COLOR_MODE_AUTO,
    .calibrated = false,
    .motor_base_speed = DEFAULT_MOTOR_BASE_SPEED,
    .motor_spin_speed = DEFAULT_MOTOR_SPIN_SPEED,
    .motor_kick_pwm   = DEFAULT_MOTOR_KICK_PWM,
    .motor_kick_time_ms = DEFAULT_MOTOR_KICK_TIME_MS,
    .motor_turn_time_ms = DEFAULT_MOTOR_TURN_TIME_MS
};

/* Status string for notifications */
static char g_status[MAX_STATUS_LEN] = "Ready";

/* Connection handle */
static hci_con_handle_t g_connection_handle = HCI_CON_HANDLE_INVALID;

/* Advertisement data */
static const uint8_t adv_data[] = {
    /* Flags: General Discoverable + BR/EDR Not Supported */
    0x02, BLUETOOTH_DATA_TYPE_FLAGS, 0x06,
    /* Complete Local Name */
    0x0A, BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME, 'R', 'o', 'b', 'o', 'M', 'o', 'v', 'e', 'l',
    /* 16-bit Service UUID: 0xFF10 */
    0x03, BLUETOOTH_DATA_TYPE_COMPLETE_LIST_OF_16_BIT_SERVICE_CLASS_UUIDS, 0x10, 0xFF,
};
static const uint8_t adv_data_len = sizeof(adv_data);

/* Packet handler registration */
static btstack_packet_callback_registration_t hci_event_callback_registration;

/* ==========================================================================
 * Command Parser
 * ========================================================================== */

/**
 * @brief Parse BLE command and update robot state
 */
static void parse_command(const uint8_t* data, uint16_t len)
{
    /* Null-terminate the string */
    char cmd[32] = {0};
    if (len >= sizeof(cmd)) len = sizeof(cmd) - 1;
    memcpy(cmd, data, len);
    cmd[len] = '\0';
    
    /* Convert to lowercase for comparison */
    for (int i = 0; cmd[i]; i++) {
        if (cmd[i] >= 'A' && cmd[i] <= 'Z') {
            cmd[i] += 32;
        }
    }
    
    printf("[BLE] Command received: %s\n", cmd);
    
    if (strcmp(cmd, "start") == 0) {
        g_robot_state.running = true;
        printf("[BLE] Robot started\n");
    }
    else if (strcmp(cmd, "stop") == 0) {
        g_robot_state.running = false;
        printf("[BLE] Robot stopped\n");
    }
    else if (strcmp(cmd, "azul") == 0 || strcmp(cmd, "blue") == 0) {
        g_robot_state.color_mode = COLOR_MODE_BLUE;
        printf("[BLE] Mode: BLUE only\n");
    }
    else if (strcmp(cmd, "vermelho") == 0 || strcmp(cmd, "red") == 0) {
        g_robot_state.color_mode = COLOR_MODE_RED;
        printf("[BLE] Mode: RED only\n");
    }
    else if (strcmp(cmd, "amarelo") == 0 || strcmp(cmd, "yellow") == 0) {
        g_robot_state.color_mode = COLOR_MODE_YELLOW;
        printf("[BLE] Mode: YELLOW only\n");
    }
    else if (strcmp(cmd, "auto") == 0) {
        g_robot_state.color_mode = COLOR_MODE_AUTO;
        printf("[BLE] Mode: AUTO (priority)\n");
    }
    /* Parameter commands: "key value" */
    else if (strncmp(cmd, "base_speed", 10) == 0) {
        int val = 0;
        if (sscanf(cmd + 10, "%d", &val) == 1) {
            g_robot_state.motor_base_speed = (uint16_t)val;
            printf("[BLE] Base Speed: %d\n", val);
        }
    }
    else if (strncmp(cmd, "spin_speed", 10) == 0) {
        int val = 0;
        if (sscanf(cmd + 10, "%d", &val) == 1) {
            g_robot_state.motor_spin_speed = (uint16_t)val;
            printf("[BLE] Spin Speed: %d\n", val);
        }
    }
    else if (strncmp(cmd, "kick_pwm", 8) == 0) {
        int val = 0;
        if (sscanf(cmd + 8, "%d", &val) == 1) {
            g_robot_state.motor_kick_pwm = (uint16_t)val;
            printf("[BLE] Kick PWM: %d\n", val);
        }
    }
    else if (strncmp(cmd, "kick_ms", 7) == 0) {
        int val = 0;
        if (sscanf(cmd + 7, "%d", &val) == 1) {
            g_robot_state.motor_kick_time_ms = (uint16_t)val;
            printf("[BLE] Kick Time: %d ms\n", val);
        }
    }
    else if (strncmp(cmd, "turn_ms", 7) == 0) {
        int val = 0;
        if (sscanf(cmd + 7, "%d", &val) == 1) {
            g_robot_state.motor_turn_time_ms = (uint16_t)val;
            printf("[BLE] Turn Time: %d ms\n", val);
        }
    }
    else {
        printf("[BLE] Unknown command: %s\n", cmd);
    }
}

/* ==========================================================================
 * GATT Callbacks
 * ========================================================================== */

static uint16_t att_read_callback(
    hci_con_handle_t connection_handle,
    uint16_t att_handle,
    uint16_t offset,
    uint8_t* buffer,
    uint16_t buffer_size)
{
    (void)connection_handle;
    
    /* Status characteristic */
    if (att_handle == ATT_CHARACTERISTIC_0000FF12_0000_1000_8000_00805F9B34FB_01_VALUE_HANDLE) {
        uint16_t len = strlen(g_status);
        if (buffer) {
            memcpy(buffer, g_status + offset, len - offset);
        }
        return len - offset;
    }
    
    return 0;
}

static int att_write_callback(
    hci_con_handle_t connection_handle,
    uint16_t att_handle,
    uint16_t transaction_mode,
    uint16_t offset,
    uint8_t* buffer,
    uint16_t buffer_size)
{
    (void)connection_handle;
    (void)transaction_mode;
    (void)offset;
    
    /* Command characteristic */
    if (att_handle == ATT_CHARACTERISTIC_0000FF11_0000_1000_8000_00805F9B34FB_01_VALUE_HANDLE) {
        parse_command(buffer, buffer_size);
        return 0;
    }
    
    return 0;
}

/* ==========================================================================
 * HCI Event Handler
 * ========================================================================== */

static void packet_handler(uint8_t packet_type, uint16_t channel, uint8_t* packet, uint16_t size)
{
    (void)channel;
    (void)size;
    
    if (packet_type != HCI_EVENT_PACKET) return;
    
    uint8_t event_type = hci_event_packet_get_type(packet);
    
    switch (event_type) {
        case BTSTACK_EVENT_STATE:
            if (btstack_event_state_get_state(packet) == HCI_STATE_WORKING) {
                bd_addr_t local_addr;
                gap_local_bd_addr(local_addr);
                printf("[BLE] BTstack started, address: %s\n", bd_addr_to_str(local_addr));
                
                /* Start advertising */
                uint16_t adv_int_min = ADV_INTERVAL_MIN;
                uint16_t adv_int_max = ADV_INTERVAL_MAX;
                uint8_t adv_type = 0;  /* ADV_IND */
                bd_addr_t null_addr = {0};
                gap_advertisements_set_params(adv_int_min, adv_int_max, adv_type, 0, null_addr, 0x07, 0x00);
                gap_advertisements_set_data(adv_data_len, (uint8_t*)adv_data);
                gap_advertisements_enable(1);
                
                printf("[BLE] Advertising started as '%s'\n", ROBOT_NAME);
            }
            break;
            
        case HCI_EVENT_LE_META:
            switch (hci_event_le_meta_get_subevent_code(packet)) {
                case HCI_SUBEVENT_LE_CONNECTION_COMPLETE:
                    g_connection_handle = hci_subevent_le_connection_complete_get_connection_handle(packet);
                    printf("[BLE] Client connected (handle: 0x%04X)\n", g_connection_handle);
                    break;
            }
            break;
            
        case HCI_EVENT_DISCONNECTION_COMPLETE:
            g_connection_handle = HCI_CON_HANDLE_INVALID;
            printf("[BLE] Client disconnected, restarting advertising\n");
            gap_advertisements_enable(1);
            break;
    }
}

/* ==========================================================================
 * Public API
 * ========================================================================== */

void ble_server_init(void)
{
    printf("[BLE] Initializing BTstack...\n");
    
    /* Initialize L2CAP */
    l2cap_init();
    
    /* Initialize Security Manager */
    sm_init();
    
    /* Initialize ATT Server */
    att_server_init(profile_data, att_read_callback, att_write_callback);
    
    /* Register HCI event handler */
    hci_event_callback_registration.callback = &packet_handler;
    hci_add_event_handler(&hci_event_callback_registration);
    
    /* Register ATT event handler */
    att_server_register_packet_handler(packet_handler);
    
    /* Turn on Bluetooth */
    hci_power_control(HCI_POWER_ON);
    
    printf("[BLE] BTstack initialized\n");
}

void ble_server_process(void)
{
    /* BTstack uses async processing via callbacks, no polling needed */
}

RobotState* ble_get_robot_state(void)
{
    return &g_robot_state;
}

void ble_update_status(const char* status)
{
    strncpy(g_status, status, MAX_STATUS_LEN - 1);
    g_status[MAX_STATUS_LEN - 1] = '\0';
    
    /* Send notification if connected */
    if (g_connection_handle != HCI_CON_HANDLE_INVALID) {
        att_server_request_can_send_now_event(g_connection_handle);
    }
}
