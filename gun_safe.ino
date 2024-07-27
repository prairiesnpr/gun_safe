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

DHT_Unified dht(DHTPIN, DHTTYPE);

void (*resetFunc)(void) = 0;

auto timer = timer_create_default(); // create a timer with default settings

unsigned long loop_time = millis();
unsigned long last_msg_time = loop_time - 1000;

bool doorLastState = 0;
unsigned long lastDoorDebounceTime = 0;
unsigned long debounceDelay = 6;

SoftwareSerial nss(ssRX, ssTX);

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

  // Reset xbee
  digitalWrite(XBEE_RST, LOW);
  pinMode(XBEE_RST, INPUT);

  Serial.begin(9600);
  Serial.println(F("Startup"));
  dht.begin();
  nss.begin(9600);
  zha.Start(nss, zdoReceive, NUM_ENDPOINTS, ENDPOINTS);

  // Set up callbacks
  zha.registerCallbacks(atCmdResp, zbTxStatusResp, otherResp);

  Serial.println(F("CB Conf"));

  timer.every(30000, update_sensors);
  wdt_enable(WDTO_8S);
}

void update_temp()
{
  if (zha.dev_status == READY)
  {
    Endpoint end_point = zha.GetEndpoint(TEMP_ENDPOINT);
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
      attribute *t_attr = t_cluster.GetAttr(CURRENT_STATE);
      int16_t cor_t = (int16_t)(event.temperature * 100.0);
      t_attr->SetValue(cor_t);
      Serial.print((int16_t)t_attr->GetIntValue());
      Serial.println(F("°C"));
      zha.sendAttributeRpt(t_cluster.id, t_attr, end_point.id, 1);
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
      attribute *h_attr = h_cluster.GetAttr(CURRENT_STATE);
      uint16_t cor_h = (uint16_t)(event.relative_humidity * 100.0);
      h_attr->SetValue(cor_h);
      zha.sendAttributeRpt(h_cluster.id, h_attr, end_point.id, 1);
    }
  }
}

bool update_sensors(void *)
{
  update_temp();
  return true;
}

void set_led()
{
  // Get current brightness
  Endpoint end_point = zha.GetEndpoint(LIGHT_ENDPOINT);
  Cluster clr_clstr = end_point.GetCluster(COLOR_CLUSTER_ID);
  attribute *clr_x_attr = clr_clstr.GetAttr(ATTR_CURRENT_X);
  attribute *clr_y_attr = clr_clstr.GetAttr(ATTR_CURRENT_Y);
  Cluster lvl_clstr = end_point.GetCluster(LEVEL_CONTROL_CLUSTER_ID);
  attribute *lvl_attr = lvl_clstr.GetAttr(CURRENT_STATE);
  uint16_t color_x = clr_x_attr->GetIntValue();
  uint16_t color_y = clr_x_attr->GetIntValue();
  uint8_t ibrightness = lvl_attr->GetIntValue();
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
}

void SetClrAttr(uint8_t ep_id, uint16_t cluster_id, uint16_t color_x, uint16_t color_y, uint8_t rqst_seq_id)
{
  Endpoint end_point = zha.GetEndpoint(ep_id);
  Cluster cluster = end_point.GetCluster(cluster_id);
  attribute *attr;

  Cluster on_off_clstr = end_point.GetCluster(ON_OFF_CLUSTER_ID);
  attribute *on_off_attr = on_off_clstr.GetAttr(CURRENT_STATE);

  Cluster lvl_clstr = end_point.GetCluster(LEVEL_CONTROL_CLUSTER_ID);
  attribute *lvl_attr = lvl_clstr.GetAttr(CURRENT_STATE);
  on_off_attr->SetValue(0x01);
  zha.sendAttributeWriteRsp(ON_OFF_CLUSTER_ID, on_off_attr, ep_id, 1, 0x01, rqst_seq_id);

  Serial.print("Clstr: ");
  Serial.println(cluster_id, HEX);
  if (cluster_id == COLOR_CLUSTER_ID)
  {
    attr = cluster.GetAttr(ATTR_CURRENT_X);
    attr->SetValue(color_x);
    attr = cluster.GetAttr(ATTR_CURRENT_Y);
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
  attribute *attr = cluster.GetAttr(attr_id);
  if (cluster_id == ON_OFF_CLUSTER_ID)
  {
    if (value == 0x00)
    {
      zha.sendAttributeWriteRsp(cluster_id, attr, ep_id, 1, value, rqst_seq_id);
      *attr->value = value;
    }
    else if (value == 0x01)
    {
      zha.sendAttributeWriteRsp(cluster_id, attr, ep_id, 1, value, rqst_seq_id);
      *attr->value = value;
    }
    else
    {
      Serial.print(F("Ukn SetAttr"));
    }
  }
  else if (cluster_id == LEVEL_CONTROL_CLUSTER_ID)
  {
    zha.sendAttributeWriteRsp(cluster_id, attr, ep_id, 1, value, rqst_seq_id);
    *attr->value = value;
    set_led();
  }
  else
  {
    Serial.print(F("Ukn SetAttr"));
  }
}
void loop()
{
  zha.loop();

  if (zha.dev_status == READY)
  {
    uint8_t door_state = digitalRead(DOOR_PIN) ^ 1;
    uint8_t ias_state = digitalRead(VIBRATION_PIN);

    Endpoint door_end_point = zha.GetEndpoint(DOOR_ENDPOINT);
    Cluster door_cluster = door_end_point.GetCluster(BINARY_INPUT_CLUSTER_ID);
    attribute *door_attr = door_cluster.GetAttr(BINARY_PV_ATTR);

    Endpoint ias_end_point = zha.GetEndpoint(IAS_ENDPOINT);
    Cluster ias_cluster = ias_end_point.GetCluster(IAS_ZONE_CLUSTER_ID);
    attribute *ias_attr = ias_cluster.GetAttr(IAS_ZONE_STATUS);
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
        zha.sendAttributeRpt(door_cluster.id, door_attr, door_end_point.id, 1);
      }
    }

    doorLastState = door_state;
  }
  else if ((loop_time - last_msg_time) > 1000)
  {
    Serial.print(F("Not Started "));
    Serial.print(start_fails);
    Serial.print(F(" of "));
    Serial.println(START_LOOPS);

    last_msg_time = millis();
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

void zdoReceive(ZBExplicitRxResponse &erx, uintptr_t)
{
  // Create a reply packet containing the same data
  // This directly reuses the rx data array, which is ok since the tx
  // packet is sent before any new response is received

  if (erx.getRemoteAddress16() == 0)
  {
    zha.cmd_seq_id = erx.getFrameData()[erx.getDataOffset() + 1];
    Serial.print(F("Cmd Seq: "));
    Serial.println(zha.cmd_seq_id);

    uint8_t ep = erx.getDstEndpoint();
    uint16_t clId = erx.getClusterId();
    uint8_t cmd_id = erx.getFrameData()[erx.getDataOffset() + 2];
    uint8_t frame_type = erx.getFrameData()[erx.getDataOffset()] & 0x03;
    if (frame_type)
    {
      Serial.println(F("Clstr Cmd"));
      if (ep == DOOR_ENDPOINT)
      {
        Serial.println(F("Door Ep"));
      }
      else if (ep == TEMP_ENDPOINT)
      {
        Serial.println(F("Temp Ep"));
      }
      else if (ep == IAS_ENDPOINT)
      {
        Serial.println(F("IAS Ep"));
      }
      else if (ep == LIGHT_ENDPOINT)
      {
        Serial.println(F("Light Ep"));
        if (clId == ON_OFF_CLUSTER_ID)
        {
          Serial.println(F("ON/OFF"));
          Endpoint end_point = zha.GetEndpoint(ep);
          // Cluster Command, so it's a write command
          uint8_t len_data = erx.getDataLength() - 3;
          uint16_t attr_rqst[len_data / 2];
          uint8_t new_state = erx.getFrameData()[erx.getDataOffset() + 2];

          for (uint8_t i = erx.getDataOffset(); i < (erx.getDataLength() + erx.getDataOffset() + 3); i++)
          {
            Serial.print(erx.getFrameData()[i], HEX);
            Serial.print(F(" "));
          }
          Serial.println();
          SetLvlStAttr(ep, clId, CURRENT_STATE, new_state, erx.getFrameData()[erx.getDataOffset() + 1]);
        }
        if (clId == LEVEL_CONTROL_CLUSTER_ID)
        {
          Serial.println(F("Lt Lvl"));
          uint8_t len_data = erx.getDataLength() - 3;
          uint16_t attr_rqst[len_data / 2];
          for (uint8_t i = erx.getDataOffset(); i < (erx.getDataLength() + erx.getDataOffset() + 3); i++)
          {
            Serial.print(erx.getFrameData()[i], HEX);
            Serial.print(F(" "));
          }
          Serial.println();
          Endpoint end_point = zha.GetEndpoint(ep);

          SetLvlStAttr(ep, clId, CURRENT_STATE, erx.getFrameData()[3], erx.getFrameData()[erx.getDataOffset() + 1]);
        }
        if (clId == COLOR_CLUSTER_ID)
        {
          Serial.println(F("Clr"));
          uint8_t len_data = erx.getDataLength() - 3;
          uint16_t attr_rqst[len_data / 2];

          for (uint8_t i = erx.getDataOffset(); i < (erx.getDataLength() + erx.getDataOffset() + 3); i++)
          {
            Serial.print(erx.getFrameData()[i], HEX);
            Serial.print(F(" "));
          }
          Serial.println();
          Endpoint end_point = zha.GetEndpoint(ep);
          uint16_t color_x = ((uint16_t)erx.getFrameData()[21] << 8) | erx.getFrameData()[20];
          uint16_t color_y = ((uint16_t)erx.getFrameData()[23] << 8) | erx.getFrameData()[22];
          SetClrAttr(ep, clId, color_x, color_y, erx.getFrameData()[erx.getDataOffset() + 1]);
        }
      }

      if (erx.getClusterId() == BASIC_CLUSTER_ID)
      {
        Serial.println(F("Basic Clstr"));
      }
    }
    else
    {
      Serial.println(F("Glbl Cmd"));

      Endpoint end_point = zha.GetEndpoint(ep);
      Cluster cluster = end_point.GetCluster(clId);
      if (cmd_id == 0x00)
      {
        // Read attributes
        Serial.println(F("Read Attr"));
        uint8_t len_data = erx.getDataLength() - 3;
        uint16_t attr_rqst[len_data / 2];
        for (uint8_t i = erx.getDataOffset() + 3; i < (len_data + erx.getDataOffset() + 3); i += 2)
        {
          attr_rqst[i / 2] = (erx.getFrameData()[i + 1] << 8) |
                             (erx.getFrameData()[i] & 0xff);
          attribute *attr = end_point.GetCluster(erx.getClusterId()).GetAttr(attr_rqst[i / 2]);
          Serial.print(F("Clstr Rd Att: "));
          Serial.println(attr_rqst[i / 2]);
          zha.sendAttributeRsp(erx.getClusterId(), attr, ep, 0x01, 0x01, zha.cmd_seq_id);
          zha.cmd_seq_id++;
        }
      }
      else
      {
        Serial.println(F("Not Read Attr"));
      }
    }
    uint8_t frame_direction = (erx.getFrameData()[erx.getDataOffset()] >> 3) & 1;
    if (frame_direction)
    {
      Serial.println(F("Srv to Client"));
    }
    else
    {
      Serial.println(F("Client to Srv"));
    }
    Serial.print(F("ZDO: EP: "));
    Serial.print(ep);
    Serial.print(F(", Clstr: "));
    Serial.print(clId, HEX);
    Serial.print(F(" Cmd Id: "));
    Serial.print(cmd_id, HEX);
    Serial.print(F(" FrmCtl: "));
    Serial.println(erx.getFrameData()[erx.getDataOffset()], BIN);
    /*
    for (uint8_t i = 0; i<erx.getFrameDataLength(); i++) {
      Serial.print(erx.getFrameData()[i], HEX);
      Serial.print(F(" "));
    }
    Serial.println();
    */

    if (erx.getClusterId() == ACTIVE_EP_RQST)
    {
      // Have to match sequence number in response
      cmd_result = NULL;
      zha.last_seq_id = erx.getFrameData()[erx.getDataOffset()];
      zha.sendActiveEpResp(zha.last_seq_id);
    }
    if (erx.getClusterId() == SIMPLE_DESC_RQST)
    {
      Serial.print("Simple Desc Rqst, Ep: ");
      // Have to match sequence number in response
      // Payload is EndPoint
      // Can this just be regular ep?
      uint8_t ep_msg = erx.getFrameData()[erx.getDataOffset() + 3];
      Serial.println(ep_msg, HEX);
      zha.sendSimpleDescRpt(ep_msg, erx.getFrameData()[erx.getDataOffset()]);
    }
  }
}
