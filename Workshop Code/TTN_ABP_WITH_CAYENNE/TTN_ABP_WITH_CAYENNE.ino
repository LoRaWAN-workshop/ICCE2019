#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>
#include <CayenneLPP.h>
#include <SimpleDHT.h>

//Set device address
static const PROGMEM u1_t NWKSKEY[16] = { 0x2D, 0x6A, 0xCB, 0x9B, 0x78, 0xF1, 0x76, 0xFB, 0xD1, 0xF0, 0xA6, 0x5B, 0xAB, 0x73, 0xDB, 0x19 };

// LoRaWAN AppSKey, application session key
// This is the default Semtech key, which is used by the early prototype TTN
// network.
static const u1_t PROGMEM APPSKEY[16] = { 0x37, 0xCB, 0x57, 0x43, 0x61, 0x97, 0xCA, 0x13, 0xFD, 0x7A, 0xA5, 0x1B, 0x90, 0xFC, 0x7F, 0xE8 };

// LoRaWAN end-device address (DevAddr)
static const u4_t DEVADDR = 0x12345678 ; // <-- Change this address for every node!

//Set data rate (DR_SF7 - DR_SF12)
u1_t DR = DR_SF10;
//Set Tx power (dBm)
s1_t PW = 14;

SimpleDHT11 dht11;
CayenneLPP lpp(100);

byte temp = 0;
byte humid = 0;
float ldr = 0;

void os_getArtEui (u1_t* buf) { }
void os_getDevEui (u1_t* buf) { }
void os_getDevKey (u1_t* buf) { }

static osjob_t sendjob;

const unsigned TX_INTERVAL = 30;

const lmic_pinmap lmic_pins = {
  .nss = 10,
  .rxtx = LMIC_UNUSED_PIN,
  .rst = LMIC_UNUSED_PIN,
  .dio = {4, 5, 7},
};

void onEvent (ev_t ev) {
  Serial.print(os_getTime());
  Serial.print(": ");
  switch (ev) {
    case EV_SCAN_TIMEOUT:
      Serial.println(F("EV_SCAN_TIMEOUT"));
      break;
    case EV_BEACON_FOUND:
      Serial.println(F("EV_BEACON_FOUND"));
      break;
    case EV_BEACON_MISSED:
      Serial.println(F("EV_BEACON_MISSED"));
      break;
    case EV_BEACON_TRACKED:
      Serial.println(F("EV_BEACON_TRACKED"));
      break;
    case EV_JOINING:
      Serial.println(F("EV_JOINING"));
      break;
    case EV_JOINED:
      Serial.println(F("EV_JOINED"));
      break;
    case EV_RFU1:
      Serial.println(F("EV_RFU1"));
      break;
    case EV_JOIN_FAILED:
      Serial.println(F("EV_JOIN_FAILED"));
      break;
    case EV_REJOIN_FAILED:
      Serial.println(F("EV_REJOIN_FAILED"));
      break;
    case EV_TXCOMPLETE:
      Serial.println(F("EV_TXCOMPLETE (includes waiting for RX windows)"));
      if (LMIC.txrxFlags & TXRX_ACK)
        Serial.println(F("Received ack"));
      if (LMIC.dataLen) {
        Serial.println(F("Received "));
        Serial.println(LMIC.rps);
        Serial.println(LMIC.dataLen);
        Serial.println(F(" bytes of payload"));
      }
      // Schedule next transmission
      os_setTimedCallback(&sendjob, os_getTime() + sec2osticks(TX_INTERVAL), do_send);
      break;
    case EV_LOST_TSYNC:
      Serial.println(F("EV_LOST_TSYNC"));
      break;
    case EV_RESET:
      Serial.println(F("EV_RESET"));
      break;
    case EV_RXCOMPLETE:
      // data received in ping slot
      Serial.println(F("EV_RXCOMPLETE"));
      break;
    case EV_LINK_DEAD:
      Serial.println(F("EV_LINK_DEAD"));
      break;
    case EV_LINK_ALIVE:
      Serial.println(F("EV_LINK_ALIVE"));
      break;
    default:
      Serial.println(F("Unknown event"));
      break;
  }
}

void do_send(osjob_t* j) {

  dht11.read(6, &temp, &humid, NULL);
  ldr = analogRead(A0) / 10;

  lpp.reset();

  lpp.addTemperature(3, (float)temp);
  lpp.addRelativeHumidity(4, (float)humid);
  lpp.addAnalogInput(5, ldr);

  if (LMIC.opmode & OP_TXRXPEND) {
    Serial.println(F("OP_TXRXPEND, not sending"));
  } else {
    LMIC_setTxData2(1, lpp.getBuffer(), lpp.getSize(), 0);
    Serial.println(F("Packet queued"));
    Serial.print("Temperature : ");
    Serial.println(temp);
    Serial.print("Humidity : ");
    Serial.println(humid);
    Serial.print("LDR : ");
    Serial.println(ldr);
  }

}

void setup() {

  Serial.begin(115200);
  Serial.println(F("Starting"));

#ifdef VCC_ENABLE
  pinMode(VCC_ENABLE, OUTPUT);
  pinMode(A0, INPUT);
  digitalWrite(VCC_ENABLE, HIGH);
  delay(1000);
#endif

  os_init();
  LMIC_reset();
#ifdef PROGMEM
  uint8_t appskey[sizeof(APPSKEY)];
  uint8_t nwkskey[sizeof(NWKSKEY)];
  memcpy_P(appskey, APPSKEY, sizeof(APPSKEY));
  memcpy_P(nwkskey, NWKSKEY, sizeof(NWKSKEY));
  LMIC_setSession (0x1, DEVADDR, nwkskey, appskey);
#else
  LMIC_setSession (0x1, DEVADDR, NWKSKEY, APPSKEY);
#endif

  LMIC_setupChannel(0, 923200000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);  // g-band
  LMIC_setupChannel(1, 923400000, DR_RANGE_MAP(DR_SF12, DR_SF7B), BAND_CENTI);  // g-band
  LMIC_setupChannel(2, 923600000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);  // g-band
  LMIC_setupChannel(3, 923800000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);  // g-band
  LMIC_setupChannel(4, 924000000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);  // g-band
  LMIC_setupChannel(5, 924200000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);  // g-band
  LMIC_setupChannel(6, 924400000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);  // g-band
  LMIC_setupChannel(7, 924600000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);  // g-band
  LMIC_setupChannel(8, 924800000, DR_RANGE_MAP(DR_FSK,  DR_FSK),  BAND_MILLI);     // g2-band

  LMIC_setLinkCheckMode(0);

  LMIC.dn2Dr = DR_SF9;

  LMIC_setDrTxpow(DR, PW);

  do_send(&sendjob);
}

void loop() {
  os_runloop_once();
}
