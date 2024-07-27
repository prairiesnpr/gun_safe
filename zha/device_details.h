#include <stdint.h>

#define NUM_ENDPOINTS 4

static uint8_t *manuf = (uint8_t *)"iSilentLLC";
static attribute door_basic_attr[]{{0x0004, manuf, 10, ZCL_CHAR_STR}, {0x0005, (uint8_t *)"Safe", 4, ZCL_CHAR_STR}};
static attribute temp_basic_attr[]{{0x0004, manuf, 10, ZCL_CHAR_STR}, {0x0005, (uint8_t *)"Temp", 4, ZCL_CHAR_STR}};
static attribute vibration_basic_attr[]{{0x0004, manuf, 10, ZCL_CHAR_STR}, {0x0005, (uint8_t *)"Alarm", 5, ZCL_CHAR_STR}};
static attribute light_basic_attr[]{{0x0004, manuf, 10, ZCL_CHAR_STR}, {0x0005, (uint8_t *)"Light", 5, ZCL_CHAR_STR}};

static attribute door_attr[] = {
    {0x0055, 0x00, 1, ZCL_BOOL}, // present value
    {0x006F, 0x0, 1, ZCL_MAP8}   // Status flags
};
static attribute light_bool_attr[]{{0x0000, 0x00, 1, ZCL_BOOL}};
static attribute light_level_attr[]{{0x0000, 0x00, 1, ZCL_UINT8_T}};
static attribute light_color_attr[] = {
    {ATTR_CURRENT_X, 0x00, 2, ZCL_UINT16_T},      // CurrentX
    {ATTR_CURRENT_Y, 0x00, 2, ZCL_UINT16_T},      // CurrentY
    {ATTR_CURRENT_CT_MRDS, 0x00, 2, ZCL_UINT16_T} // ColorTemperatureMireds
};
static attribute vibration_attr[]{
    {0x0000, 0x00, 1, ZCL_ENUM8},               // ZoneState
    {0x0001, 0x002d, 2, ZCL_ENUM16},            // ZoneType, Vibration
    {0x0002, 0b0000000000000000, 2, ZCL_MAP16}, // ZoneStatus
};
static attribute temp_attr[] = {{0x0000, 0x00, 2, ZCL_INT16_T}};
static attribute humid_attr[] = {{0x0000, 0x00, 2, ZCL_UINT16_T}};

static Cluster door_in_clusters[] = {
    Cluster(BASIC_CLUSTER_ID, door_basic_attr, 2),
    Cluster(BINARY_INPUT_CLUSTER_ID, door_attr, 1)};
static Cluster vibration_in_clusters[] = {
    Cluster(BASIC_CLUSTER_ID, vibration_basic_attr, 2),
    Cluster(IAS_ZONE_CLUSTER_ID, vibration_attr, 3)};
static Cluster light_in_clusters[] = {
    Cluster(BASIC_CLUSTER_ID, light_basic_attr, 2),
    Cluster(ON_OFF_CLUSTER_ID, light_bool_attr, 1),
    Cluster(LEVEL_CONTROL_CLUSTER_ID, light_level_attr, 1),
    Cluster(COLOR_CLUSTER_ID, light_color_attr, 3),
};
static Cluster t_in_clusters[] = {
    Cluster(BASIC_CLUSTER_ID, temp_basic_attr, 2),
    Cluster(TEMP_CLUSTER_ID, temp_attr, 1),
    Cluster(HUMIDITY_CLUSTER_ID, humid_attr, 1)};

static Cluster out_clusters[] = {};
static Endpoint ENDPOINTS[NUM_ENDPOINTS] = {
    Endpoint(1, ON_OFF_SENSOR, door_in_clusters, out_clusters, 2, 0),
    Endpoint(2, TEMPERATURE_SENSOR, t_in_clusters, out_clusters, 3, 0),
    Endpoint(3, IAS_ZONE, vibration_in_clusters, out_clusters, 2, 0),
    Endpoint(4, COLOR_LIGHT, light_in_clusters, out_clusters, 4, 0),
};
