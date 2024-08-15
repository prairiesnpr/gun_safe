#include <SoftwareSerial.h>
#include <arduino-timer.h>
#include <DHT.h>
#include <DHT_U.h>
#include <avr/wdt.h>
#include "color_functions.h"
#include <xbee_zha.h>
#include "zha/device_details.h"

#define XBEE_RST 8
#define DHTPIN 4
#define DHTTYPE DHT22
#define VIBRATION_PIN 2
#define DOOR_PIN 3
#define RED_PIN 6    // correct merged with 7 do to an error on board
#define BLUE_PIN 11  // correct
#define GREEN_PIN 10 // correct
#define WHITE_PIN 9  // correct
#define FADESPEED 5
#define ssRX 12
#define ssTX 13

#define BASIC_ENDPOINT 0
#define DOOR_ENDPOINT 1
#define TEMP_ENDPOINT 2
#define IAS_ENDPOINT 3
#define LIGHT_ENDPOINT 4

#define START_LOOPS 100

uint8_t start_fails = 0;
uint8_t init_status_sent = 0;

DHT_Unified dht(DHTPIN, DHTTYPE);

void (*resetFunc)(void) = 0;

auto timer = timer_create_default(); // create a timer with default settings

unsigned long loop_time = millis();
unsigned long last_msg_time = loop_time - 1000;

bool doorLastState = 0;
unsigned long lastDoorDebounceTime = 0;
unsigned long debounceDelay = 6;

SoftwareSerial nss(ssRX, ssTX);

#ifdef __arm__
// should use uinstd.h to define sbrk but Due causes a conflict
extern "C" char *sbrk(int incr);
#else  // __ARM__
extern char *__brkval;
#endif // __arm__

int freeMemory()
{
  char top;
#ifdef __arm__
  return &top - reinterpret_cast<char *>(sbrk(0));
#elif defined(CORE_TEENSY) || (ARDUINO > 103 && ARDUINO != 151)
  return &top - __brkval;
#else  // __arm__
  return __brkval ? &top - __brkval : &top - __malloc_heap_start;
#endif // __arm__
}

void setup()
{
  pinMode(DOOR_PIN, INPUT_PULLUP);
  pinMode(VIBRATION_PIN, INPUT);
  pinMode(XBEE_RST, OUTPUT);

  // LED Strip pins
  pinMode(RED_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(WHITE_PIN, OUTPUT);

  digitalWrite(RED_PIN, 0);
  digitalWrite(GREEN_PIN, 0);
  digitalWrite(BLUE_PIN, 0);
  digitalWrite(WHITE_PIN, 0);

  // Reset xbee
  digitalWrite(XBEE_RST, LOW);
  pinMode(XBEE_RST, INPUT);

  Serial.begin(9600);
  Serial.println(F("Startup"));
  dht.begin();
  nss.begin(9600);
  zha.Start(nss, zhaClstrCmd, zhaWriteAttr, NUM_ENDPOINTS, ENDPOINTS);

  // Set up callbacks, shouldn't have to do this here, but bad design...
  zha.registerCallbacks(atCmdResp, zbTxStatusResp, otherResp, zdoReceive);

  Serial.println(F("CB Conf"));

  timer.every(30000, update_sensors);
  wdt_enable(WDTO_8S);
}

void update_temp()
{
  if (zha.dev_status == READY)
  {
    Endpoint end_point = zha.GetEndpoint(TEMP_ENDPOINT);
    attribute *attr;
    uint8_t attr_exists;

    sensors_event_t event;
    dht.temperature().getEvent(&event);
    if (isnan(event.temperature))
    {
      Serial.println(F("Error reading temperature!"));
    }
    else
    {
      Serial.print(F("Temperature: "));
      Serial.print(event.temperature);
      Serial.println(F("°C"));
      Cluster t_cluster = end_point.GetCluster(TEMP_CLUSTER_ID);
      attr_exists = t_cluster.GetAttr(&attr, CURRENT_STATE);
      int16_t cor_t = (int16_t)(event.temperature * 100.0);
      attr->SetValue(cor_t);
      zha.sendAttributeRpt(t_cluster.id, attr, end_point.id, 1);
    }

    dht.humidity().getEvent(&event);
    if (isnan(event.relative_humidity))
    {
      Serial.println(F("Error reading humidity!"));
    }
    else
    {
      Serial.print(F("Humidity: "));
      Serial.print(event.relative_humidity);
      Serial.println(F("%"));
      Cluster h_cluster = end_point.GetCluster(HUMIDITY_CLUSTER_ID);
      attr_exists = h_cluster.GetAttr(&attr, CURRENT_STATE);
      uint16_t cor_h = (uint16_t)(event.relative_humidity * 100.0);
      attr->SetValue(cor_h);
      zha.sendAttributeRpt(h_cluster.id, attr, end_point.id, 1);
    }
  }
}

bool update_sensors(void *)
{
  update_temp();
  return true;
}

void report_color_state()
{
  Serial.println(F("RPT CLRS"));

  Endpoint light_end_point = zha.GetEndpoint(LIGHT_ENDPOINT);
  attribute *attr;
  uint8_t attr_exists;
  Cluster cluster = light_end_point.GetCluster(COLOR_CLUSTER_ID);
  uint16_t color_ids[] = {ATTR_CURRENT_X, ATTR_CURRENT_Y};
  attr_exists = cluster.GetAttr(&attr, ATTR_CURRENT_X);

  zha.sendAttributeRptMult(&cluster, color_ids, 2, light_end_point.id, 1);
}

void report_lvl_state()
{
  Endpoint light_end_point = zha.GetEndpoint(LIGHT_ENDPOINT);
  attribute *attr;
  uint8_t attr_exists;
  Cluster cluster = light_end_point.GetCluster(LEVEL_CONTROL_CLUSTER_ID);
  attr_exists = cluster.GetAttr(&attr, CURRENT_STATE);
  zha.sendAttributeRpt(cluster.id, attr, light_end_point.id, 1);
}

void set_led()
{
  // Get current brightness
  Endpoint end_point = zha.GetEndpoint(LIGHT_ENDPOINT);
  Cluster clr_clstr = end_point.GetCluster(COLOR_CLUSTER_ID);
  Cluster lvl_clstr = end_point.GetCluster(LEVEL_CONTROL_CLUSTER_ID);
  Cluster on_off_clstr = end_point.GetCluster(ON_OFF_CLUSTER_ID);
  uint8_t attr_exists;

  attribute *clr_x_attr;
  attr_exists = clr_clstr.GetAttr(&clr_x_attr, ATTR_CURRENT_X);

  attribute *clr_y_attr;
  attr_exists = clr_clstr.GetAttr(&clr_y_attr, ATTR_CURRENT_Y);

  attribute *lvl_attr;
  attr_exists = lvl_clstr.GetAttr(&lvl_attr, CURRENT_STATE);

  attribute *on_off_attr;
  attr_exists = on_off_clstr.GetAttr(&on_off_attr, CURRENT_STATE);

  uint16_t color_x = clr_x_attr->GetIntValue(0x00);
  uint16_t color_y = clr_y_attr->GetIntValue(0x00);
  uint8_t ibrightness = lvl_attr->GetIntValue(0x00);
  Serial.print(F("Color: X: "));
  Serial.print(color_x, DEC);
  Serial.print(F(" Y: "));
  Serial.print(color_y, DEC);
  Serial.print(F(" B: "));
  Serial.println(ibrightness, DEC);

  RGBW result = color_int_xy_brightness_to_rgb(color_x, color_y, ibrightness);

  Serial.print(F("R: "));
  Serial.print(result.r, DEC);
  Serial.print(F(" G: "));
  Serial.print(result.g, DEC);
  Serial.print(F(" B: "));
  Serial.print(result.b, DEC);
  Serial.print(F(" W: "));
  Serial.print(result.w, DEC);
  Serial.println();
  if (on_off_attr->GetIntValue())
  {
    Serial.println(F("Apply RGB"));
    result.r = pow(0xff, result.r / (float)0xff);
    result.g = pow(0xff, result.g / (float)0xff);
    result.b = pow(0xff, result.b / (float)0xff);
    result.w = pow(0xff, result.w / (float)0xff);

    analogWrite(GREEN_PIN, result.g);
    analogWrite(BLUE_PIN, result.b);
    analogWrite(RED_PIN, result.r);
    analogWrite(WHITE_PIN, result.w);
  }
  else
  {
    Serial.println(F("All LED OFF"));
    analogWrite(GREEN_PIN, 0);
    analogWrite(BLUE_PIN, 0);
    analogWrite(RED_PIN, 0);
    analogWrite(WHITE_PIN, 0);
  }
}

void SetClrAttr(uint8_t ep_id, uint16_t cluster_id, uint16_t color_x, uint16_t color_y, uint8_t rqst_seq_id)
{
  Endpoint end_point = zha.GetEndpoint(ep_id);
  Cluster cluster = end_point.GetCluster(cluster_id);
  attribute *attr;
  uint8_t attr_exists;

  cluster = end_point.GetCluster(ON_OFF_CLUSTER_ID);
  attribute *on_off_attr;
  attr_exists = cluster.GetAttr(&on_off_attr, CURRENT_STATE);

  cluster = end_point.GetCluster(LEVEL_CONTROL_CLUSTER_ID);
  attribute *lvl_attr;
  attr_exists = cluster.GetAttr(&lvl_attr, CURRENT_STATE);

  on_off_attr->SetValue(0x01);
  zha.sendAttributeCmdRsp(ON_OFF_CLUSTER_ID, on_off_attr, ep_id, 1, 0x01, rqst_seq_id);

  Serial.print("Clstr: ");
  Serial.println(cluster_id, HEX);
  if (cluster_id == COLOR_CLUSTER_ID)
  {
    cluster = end_point.GetCluster(cluster_id);

    attr_exists = cluster.GetAttr(&attr, ATTR_CURRENT_X);
    attr->SetValue(color_x);
    attr_exists = cluster.GetAttr(&attr, ATTR_CURRENT_Y);
    attr->SetValue(color_y);
    set_led();
  }
  else
  {
    Serial.print(F("Ukn SetAttr"));
  }
}

void SetLvlStAttr(uint8_t ep_id, uint16_t cluster_id, uint16_t attr_id, uint8_t value, uint8_t rqst_seq_id)
{
  Endpoint end_point = zha.GetEndpoint(ep_id);
  Cluster cluster = end_point.GetCluster(cluster_id);
  attribute *attr;
  uint8_t attr_exists = cluster.GetAttr(&attr, attr_id);
  Serial.print(F("Val: "));
  Serial.println(value, HEX);
  if (cluster_id == ON_OFF_CLUSTER_ID)
  {
    if (value == 0x00 || value == 0x01)
    {
      zha.sendAttributeCmdRsp(cluster_id, attr, ep_id, 1, value, rqst_seq_id);
      attr->SetValue(value);
      set_led();
    }
    else
    {
      Serial.print(F("Bad Val"));
    }
  }
  else if (cluster_id == LEVEL_CONTROL_CLUSTER_ID)
  {
    zha.sendAttributeCmdRsp(cluster_id, attr, ep_id, 1, value, rqst_seq_id);
    attr->SetValue(value);
    set_led();
  }
  else
  {
    Serial.print(F("Ukn Cmd Attr"));
  }
}

void send_inital_state()
{
  attribute *attr;
  uint8_t attr_exists;
  uint8_t door_state = digitalRead(DOOR_PIN);
  uint8_t ias_state = digitalRead(VIBRATION_PIN);

  Endpoint light_end_point = zha.GetEndpoint(LIGHT_ENDPOINT);
  Cluster on_off_clstr = light_end_point.GetCluster(ON_OFF_CLUSTER_ID);

  attr_exists = on_off_clstr.GetAttr(&attr, CURRENT_STATE);
  zha.sendAttributeRpt(on_off_clstr.id, attr, light_end_point.id, 1);

  Endpoint door_end_point = zha.GetEndpoint(DOOR_ENDPOINT);
  Cluster door_cluster = door_end_point.GetCluster(BINARY_INPUT_CLUSTER_ID);
  attr_exists = door_cluster.GetAttr(&attr, BINARY_PV_ATTR);
  zha.sendAttributeRpt(door_cluster.id, attr, door_end_point.id, 1);

  Endpoint ias_end_point = zha.GetEndpoint(IAS_ENDPOINT);
  Cluster ias_cluster = ias_end_point.GetCluster(IAS_ZONE_CLUSTER_ID);
  attr_exists = ias_cluster.GetAttr(&attr, IAS_ZONE_STATUS);
  zha.sendAttributeRpt(ias_cluster.id, attr, ias_end_point.id, 1);
 
  report_lvl_state();
  report_color_state();
}

void loop()
{
  zha.loop();

  if (zha.dev_status == READY)
  {

    uint8_t door_state = digitalRead(DOOR_PIN);
    uint8_t ias_state = digitalRead(VIBRATION_PIN);

    Endpoint light_end_point = zha.GetEndpoint(LIGHT_ENDPOINT);
    Cluster on_off_clstr = light_end_point.GetCluster(ON_OFF_CLUSTER_ID);
    attribute *on_off_attr;
    uint8_t attr_exists;
    attr_exists = on_off_clstr.GetAttr(&on_off_attr, CURRENT_STATE);

    Endpoint door_end_point = zha.GetEndpoint(DOOR_ENDPOINT);
    Cluster door_cluster = door_end_point.GetCluster(BINARY_INPUT_CLUSTER_ID);
    attribute *door_attr;
    attr_exists = door_cluster.GetAttr(&door_attr, BINARY_PV_ATTR);

    Endpoint ias_end_point = zha.GetEndpoint(IAS_ENDPOINT);
    Cluster ias_cluster = ias_end_point.GetCluster(IAS_ZONE_CLUSTER_ID);
    attribute *ias_attr;
    attr_exists = ias_cluster.GetAttr(&ias_attr, IAS_ZONE_STATUS);
    uint16_t ias_map_state = (uint16_t)ias_state << 1;
    uint16_t cur_ias_state = (uint16_t)ias_attr->GetIntValue();

    if (ias_map_state != cur_ias_state)
    {
      Serial.print(F("EP"));
      Serial.print(ias_end_point.id);
      Serial.print(F(": "));
      Serial.print(cur_ias_state, HEX);
      Serial.print(F(" Now "));
      ias_attr->SetValue(ias_map_state);
      Serial.println(ias_map_state, HEX);
      zha.sendAttributeRpt(ias_cluster.id, ias_attr, ias_end_point.id, 1);
    }

    if (door_state != doorLastState)
    {
      lastDoorDebounceTime = millis();
    }

    if ((millis() - lastDoorDebounceTime) > debounceDelay)
    {
      if (door_state != door_attr->GetIntValue())
      {
        Serial.print(F("EP"));
        Serial.print(door_end_point.id);
        Serial.print(F(": "));
        Serial.print(door_attr->GetIntValue());
        Serial.print(F(" Now "));
        door_attr->SetValue(door_state);
        Serial.println(door_attr->GetIntValue());

        if (door_state)
        {
          on_off_attr->SetValue(0x01);
        }
        else
        {
          on_off_attr->SetValue(0x00);
        }

        zha.sendAttributeRpt(door_cluster.id, door_attr, door_end_point.id, 1);
        zha.sendAttributeRpt(on_off_clstr.id, on_off_attr, light_end_point.id, 1);
        set_led();
      }
    }

    doorLastState = door_state;
    if (!init_status_sent)
    {
      Serial.println(F("Snd Init States"));
      send_inital_state();
      init_status_sent = 1;
    }
  }
  else if ((loop_time - last_msg_time) > 1000)
  {
    Serial.print(F("Not Started "));
    Serial.print(start_fails);
    Serial.print(F(" of "));
    Serial.println(START_LOOPS);

    last_msg_time = millis();
    if (start_fails > 15)
    {
      // Sometimes we don't get a response from dev ann, try a transmit and see if we are good
      send_inital_state();
    }
    if (start_fails > START_LOOPS)
    {
      resetFunc();
    }
    start_fails++;
  }

  timer.tick();
  wdt_reset();
  loop_time = millis();
}

void zhaWriteAttr(ZBExplicitRxResponse &erx)
{

  Serial.println(F("Write Cmd"));
  // No supported attributes
}

void zhaClstrCmd(ZBExplicitRxResponse &erx)
{
  Serial.println(F("Clstr Cmd"));
  if (erx.getDstEndpoint() == DOOR_ENDPOINT)
  {
    Serial.println(F("Door Ep"));
  }
  else if (erx.getDstEndpoint() == TEMP_ENDPOINT)
  {
    Serial.println(F("Temp Ep"));
  }
  else if (erx.getDstEndpoint() == IAS_ENDPOINT)
  {
    Serial.println(F("IAS Ep"));
  }
  else if (erx.getDstEndpoint() == LIGHT_ENDPOINT)
  {
    Serial.println(F("Light Ep"));
    if (erx.getClusterId() == ON_OFF_CLUSTER_ID)
    {
      Serial.println(F("ON/OFF"));
      uint8_t new_state = erx.getFrameData()[erx.getDataOffset() + 2];

      for (uint8_t i = erx.getDataOffset(); i < (erx.getDataLength() + erx.getDataOffset() + 3); i++)
      {
        Serial.print(erx.getFrameData()[i], HEX);
        Serial.print(F(" "));
      }
      Serial.println();
      SetLvlStAttr(erx.getDstEndpoint(), erx.getClusterId(), CURRENT_STATE, new_state, erx.getFrameData()[erx.getDataOffset() + 1]);
    }
    if (erx.getClusterId() == LEVEL_CONTROL_CLUSTER_ID)
    {
      Serial.println(F("Lt Lvl"));
      for (uint8_t i = erx.getDataOffset(); i < (erx.getDataLength() + erx.getDataOffset() + 3); i++)
      {
        Serial.print(erx.getFrameData()[i], HEX);
        Serial.print(F(" "));
      }
      Serial.println();
      uint8_t new_state = erx.getFrameData()[erx.getDataOffset() + 3];

      SetLvlStAttr(erx.getDstEndpoint(), erx.getClusterId(), CURRENT_STATE, new_state, erx.getFrameData()[erx.getDataOffset() + 1]);
    }
    if (erx.getClusterId() == COLOR_CLUSTER_ID)
    {
      Serial.println(F("Clr"));
      for (uint8_t i = erx.getDataOffset(); i < (erx.getDataLength() + erx.getDataOffset() + 3); i++)
      {
        Serial.print(erx.getFrameData()[i], HEX);
        Serial.print(F(" "));
      }
      Serial.println();
      uint16_t color_x = ((uint16_t)erx.getFrameData()[21] << 8) | erx.getFrameData()[20];
      uint16_t color_y = ((uint16_t)erx.getFrameData()[23] << 8) | erx.getFrameData()[22];
      SetClrAttr(erx.getDstEndpoint(), erx.getClusterId(), color_x, color_y, erx.getFrameData()[erx.getDataOffset() + 1]);
    }
  }

  if (erx.getClusterId() == BASIC_CLUSTER_ID)
  {
    Serial.println(F("Basic Clstr"));
  }
}
