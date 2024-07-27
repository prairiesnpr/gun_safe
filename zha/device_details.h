#include <stdint.h>

#define NUM_ENDPOINTS 4

static uint8_t *manuf = (uint8_t *)"iSilentLLC";
static attribute door_basic_attr[]{
    {MANUFACTURER_ATTR, manuf, 10, ZCL_CHAR_STR},
    {MODEL_ATTR, (uint8_t *)"Safe", 4, ZCL_CHAR_STR}};
static attribute temp_basic_attr[]{
    {MANUFACTURER_ATTR, manuf, 10, ZCL_CHAR_STR},
    {MODEL_ATTR, (uint8_t *)"Temp", 4, ZCL_CHAR_STR}};
static attribute vibration_basic_attr[]{
    {MANUFACTURER_ATTR, manuf, 10, ZCL_CHAR_STR},
    {MODEL_ATTR, (uint8_t *)"Alarm", 5, ZCL_CHAR_STR}};
static attribute light_basic_attr[]{
    {MANUFACTURER_ATTR, manuf, 10, ZCL_CHAR_STR},
    {MODEL_ATTR, (uint8_t *)"Light", 5, ZCL_CHAR_STR}};

static attribute door_attr[] = {
    {BINARY_PV_ATTR, 0x00, 1, ZCL_BOOL},  // present value
    {BINARY_STATUS_FLG, 0x0, 1, ZCL_MAP8} // Status flags
};
static attribute light_bool_attr[]{
    {CURRENT_STATE, 0x00, 1, ZCL_BOOL}};
static attribute light_level_attr[]{
    {CURRENT_STATE, 0x00, 1, ZCL_UINT8_T}};
static attribute light_color_attr[] = {
    {ATTR_CURRENT_X, 0x00, 2, ZCL_UINT16_T},      // CurrentX
    {ATTR_CURRENT_Y, 0x00, 2, ZCL_UINT16_T},      // CurrentY
    {ATTR_CURRENT_CT_MRDS, 0x00, 2, ZCL_UINT16_T} // ColorTemperatureMireds
};
static attribute vibration_attr[]{
    {IAS_ZONE_STATE, 0x00, 1, ZCL_ENUM8},                // ZoneState
    {IAS_ZONE_TYPE, 0x002d, 2, ZCL_ENUM16},              // ZoneType, Vibration
    {IAS_ZONE_STATUS, 0b0000000000000000, 2, ZCL_MAP16}, // ZoneStatus
};
static attribute temp_attr[] = {{CURRENT_STATE, 0x00, 2, ZCL_INT16_T}};
static attribute humid_attr[] = {{CURRENT_STATE, 0x00, 2, ZCL_UINT16_T}};

static Cluster door_in_clusters[] = {
    Cluster(BASIC_CLUSTER_ID, door_basic_attr, sizeof(door_basic_attr) / sizeof(*door_basic_attr)),
    Cluster(BINARY_INPUT_CLUSTER_ID, door_attr, sizeof(door_attr) / sizeof(*door_attr))};
static Cluster vibration_in_clusters[] = {
    Cluster(BASIC_CLUSTER_ID, vibration_basic_attr, sizeof(vibration_basic_attr) / sizeof(*vibration_basic_attr)),
    Cluster(IAS_ZONE_CLUSTER_ID, vibration_attr, sizeof(vibration_attr) / sizeof(*vibration_attr))};
static Cluster light_in_clusters[] = {
    Cluster(BASIC_CLUSTER_ID, light_basic_attr, sizeof(light_basic_attr) / sizeof(*light_basic_attr)),
    Cluster(ON_OFF_CLUSTER_ID, light_bool_attr, sizeof(light_bool_attr) / sizeof(*light_bool_attr)),
    Cluster(LEVEL_CONTROL_CLUSTER_ID, light_level_attr, sizeof(light_level_attr) / sizeof(*light_level_attr)),
    Cluster(COLOR_CLUSTER_ID, light_color_attr, sizeof(light_color_attr) / sizeof(*light_color_attr)),
};
static Cluster t_in_clusters[] = {
    Cluster(BASIC_CLUSTER_ID, temp_basic_attr, sizeof(temp_basic_attr) / sizeof(*temp_basic_attr)),
    Cluster(TEMP_CLUSTER_ID, temp_attr, sizeof(temp_attr) / sizeof(*temp_attr)),
    Cluster(HUMIDITY_CLUSTER_ID, humid_attr, sizeof(humid_attr) / sizeof(*humid_attr))};

static Cluster out_clusters[] = {};
static Endpoint ENDPOINTS[NUM_ENDPOINTS] = {
    Endpoint(1, ON_OFF_SENSOR, door_in_clusters, out_clusters, sizeof(door_in_clusters) / sizeof(*door_in_clusters), 0),
    Endpoint(2, TEMPERATURE_SENSOR, t_in_clusters, out_clusters, sizeof(t_in_clusters) / sizeof(*t_in_clusters), 0),
    Endpoint(3, IAS_ZONE, vibration_in_clusters, out_clusters, sizeof(vibration_in_clusters) / sizeof(*vibration_in_clusters), 0),
    Endpoint(4, COLOR_LIGHT, light_in_clusters, out_clusters, sizeof(light_in_clusters) / sizeof(*light_in_clusters), 0),
};
