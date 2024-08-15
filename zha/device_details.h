#include <stdint.h>

#define NUM_ENDPOINTS 4


constexpr uint8_t one_zero_byte[] = {0x00};
constexpr uint8_t one_max_byte[] = {0xFF};
constexpr uint8_t two_zero_byte[] = {0x00, 0x00};
constexpr uint8_t four_zero_byte[] = {0x00, 0x00, 0x00, 0x00};
constexpr uint8_t init_color_x[] = {0x74, 0x53};
constexpr uint8_t init_color_y[] = {0x3F, 0x55};
constexpr uint8_t vib_zone_type[] ={0x2D, 0x00};
constexpr uint8_t color_cap[] = {0x08, 0x00};
constexpr uint8_t color_mode[] = {0x01};
constexpr uint8_t color_options[] = {0x00};
constexpr uint8_t enh_color_mode[] = {0x01};

constexpr char manufacturer[]  = "iSilentLLC";
constexpr char safe_model[] = "Safe";
constexpr char temp_model[] = "Temp & Humidity";
constexpr char alarm_model[] = "Alarm";
constexpr char light_model[] = "Light";

attribute BuildStringAtt(uint16_t a_id, char *value, uint8_t size, uint8_t a_type)
{
    uint8_t *value_t = (uint8_t *)value;
    return attribute(a_id, value_t, size, a_type, 0x01);
}

attribute manuf_attr = BuildStringAtt(MANUFACTURER_ATTR, const_cast<char *>(manufacturer), sizeof(manufacturer), ZCL_CHAR_STR);
attribute safe_model_attr = BuildStringAtt(MODEL_ATTR, const_cast<char *>(safe_model), sizeof(safe_model), ZCL_CHAR_STR);
attribute temp_model_attr = BuildStringAtt(MODEL_ATTR, const_cast<char *>(temp_model), sizeof(temp_model), ZCL_CHAR_STR);
attribute alarm_model_attr = BuildStringAtt(MODEL_ATTR, const_cast<char *>(alarm_model), sizeof(alarm_model), ZCL_CHAR_STR);
attribute light_model_attr = BuildStringAtt(MODEL_ATTR, const_cast<char *>(light_model), sizeof(light_model), ZCL_CHAR_STR);

static attribute door_basic_attr[]{
    manuf_attr,
    safe_model_attr};

static attribute temp_basic_attr[]{
    manuf_attr,
    temp_model_attr};

static attribute vibration_basic_attr[]{
    manuf_attr,
    alarm_model_attr};

static attribute light_basic_attr[]{
    manuf_attr,
    light_model_attr};

static attribute door_attr[] = {
    {BINARY_PV_ATTR, const_cast<uint8_t *>(one_zero_byte), 1, ZCL_BOOL},  // present value
    {BINARY_STATUS_FLG, const_cast<uint8_t *>(one_zero_byte), 1, ZCL_MAP8, 0x01} // Status flags
};
static attribute light_bool_attr[]{
    {CURRENT_STATE, const_cast<uint8_t *>(one_zero_byte), 1, ZCL_BOOL}};

static attribute light_level_attr[]{
    {CURRENT_STATE, const_cast<uint8_t *>(one_max_byte), 1, ZCL_UINT8_T}};

static attribute light_color_attr[] = {
    {ATTR_CURRENT_X, const_cast<uint8_t *>(init_color_x), 2, ZCL_UINT16_T},      // CurrentX
    {ATTR_CURRENT_Y, const_cast<uint8_t *>(init_color_y), 2, ZCL_UINT16_T},      // CurrentY
    {ATTR_COLOR_CAP, const_cast<uint8_t *>(color_cap), sizeof(color_cap), ZCL_MAP16, 0x01},
    {ATTR_COLOR_MODE, const_cast<uint8_t *>(color_mode), sizeof(color_mode), ZCL_ENUM8, 0x01},
    {ATTR_COLOR_OPT, const_cast<uint8_t *>(color_options), sizeof(color_options), ZCL_MAP8, 0x01},
    {ATTR_ENH_COLOR_MODE, const_cast<uint8_t *>(enh_color_mode), sizeof(enh_color_mode), ZCL_ENUM8, 0x01},
};
static attribute vibration_attr[]{
    {IAS_ZONE_STATE, const_cast<uint8_t *>(one_zero_byte), 1, ZCL_ENUM8},                // ZoneState
    {IAS_ZONE_TYPE, const_cast<uint8_t *>(vib_zone_type), 2, ZCL_ENUM16, 0x01},              // ZoneType, Vibration
    {IAS_ZONE_STATUS, const_cast<uint8_t *>(two_zero_byte), 2, ZCL_MAP16}, // ZoneStatus
};
static attribute temp_attr[] = {{CURRENT_STATE, const_cast<uint8_t *>(two_zero_byte), 2, ZCL_INT16_T}};
static attribute humid_attr[] = {{CURRENT_STATE, const_cast<uint8_t *>(two_zero_byte), 2, ZCL_UINT16_T}};

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
