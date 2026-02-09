/**
 * @file btstack_config.h
 * @brief BTstack configuration for RoboMovel BLE (Pico W)
 * 
 * Based on pico-examples btstack configuration
 */

#ifndef BTSTACK_CONFIG_H
#define BTSTACK_CONFIG_H

/* BLE features */
#ifndef ENABLE_BLE
#define ENABLE_BLE
#endif

#ifndef ENABLE_LE_PERIPHERAL
#define ENABLE_LE_PERIPHERAL
#endif

/* LE Device Database */
#define MAX_NR_LE_DEVICE_DB_ENTRIES 4

/* NVM (Non-Volatile Memory) settings */
#define NVM_NUM_DEVICE_DB_ENTRIES 4
#define NVM_NUM_LINK_KEYS 4

/* HCI Transport configuration for CYW43 */
#define HCI_OUTGOING_PRE_BUFFER_SIZE 4
#define HCI_ACL_CHUNK_SIZE_ALIGNMENT 4

/* HCI ACL payload */
#ifndef HCI_ACL_PAYLOAD_SIZE
#define HCI_ACL_PAYLOAD_SIZE 512
#endif

/* Connection limits */
#ifndef MAX_NR_HCI_CONNECTIONS
#define MAX_NR_HCI_CONNECTIONS 2
#endif

#ifndef MAX_NR_L2CAP_SERVICES
#define MAX_NR_L2CAP_SERVICES 2
#endif

#ifndef MAX_NR_L2CAP_CHANNELS
#define MAX_NR_L2CAP_CHANNELS 2
#endif

#ifndef MAX_NR_GATT_CLIENTS
#define MAX_NR_GATT_CLIENTS 2
#endif

#ifndef MAX_NR_SM_LOOKUP_ENTRIES
#define MAX_NR_SM_LOOKUP_ENTRIES 2
#endif

#ifndef MAX_ATT_DB_SIZE
#define MAX_ATT_DB_SIZE 256
#endif

/* Enable hexdump for debugging */
#define ENABLE_PRINTF_HEXDUMP

/* Log configuration */
#define ENABLE_LOG_INFO
#define ENABLE_LOG_ERROR

/* Use embedded run loop */
#define HAVE_EMBEDDED_TIME_MS

/* Required for Pico W BTstack */
#define CYW43_ENABLE_BLUETOOTH 1

#endif /* BTSTACK_CONFIG_H */
