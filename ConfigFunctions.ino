bool loadConfig() {
  File configFile = LittleFS.open("/config.json", "r");
  if (!configFile) {
    Serial.println("Failed to open config file");
    return false;
  }

  size_t size = configFile.size();
  if (size > 1024) {
    Serial.println("Config file size is too large");
    return false;
  }

  // Allocate a buffer to store contents of the file.
  std::unique_ptr<char[]> buf(new char[size]);

  // We don't use String here because ArduinoJson library requires the input
  // buffer to be mutable. If you don't use ArduinoJson, you may as well
  // use configFile.readString instead.
  configFile.readBytes(buf.get(), size);

  StaticJsonDocument<400> json;
  DeserializationError err = deserializeJson(json, buf.get());
  if (err) {
    Serial.print(F("Failed to parse config file: "));
    Serial.println(err.c_str());
    return false;
  }

  String ssidC      = json["ssid"];
  String passC      = json["pass"];
  String mqtt_ipC   = json["mqttServer"];
  String mqtt_portC = json["mqttPort"];
  String mqtt_authC = json["mqttAuth"];
  String mqtt_passC = json["mqttPass"];
  String mqtt_TempC = json["mqttTemp"];
  String mqtt_HumC  = json["mqttHum"];
  String mqtt_PressC= json["mqttPress"];
  String mqtt_CO2C  = json["mqttCO2"];

  otaFlag   = int(json["otaFlag"]);
  TIMEZONE  = int(json["TIMEZONE"]);

  ssid      = ssidC;
  pass      = passC;
  mqtt_ip   = mqtt_ipC;
  mqtt_port = mqtt_portC;
  mqtt_auth = mqtt_authC;
  mqtt_pass = mqtt_passC;
  mqtt_Temp = mqtt_TempC;
  mqtt_Hum  = mqtt_HumC;
  mqtt_Press= mqtt_PressC;
  mqtt_CO2  = mqtt_CO2C; 
  
  return true;
}

bool saveConfig() {
  StaticJsonDocument<400> json;

  json["otaFlag"]    = otaFlag;
  json["TIMEZONE"]   = TIMEZONE;
  json["ssid"]       = ssid;
  json["pass"]       = pass;
  json["mqttServer"] = mqtt_ip;
  json["mqttPort"]   = mqtt_port;
  json["mqttAuth"]   = mqtt_auth;
  json["mqttPass"]   = mqtt_pass;
  json["mqttTemp"]   = mqtt_Temp;
  json["mqttHum"]    = mqtt_Hum;
  json["mqttPress"]  = mqtt_Press;
  json["mqttCO2"]    = mqtt_CO2;

  File configFile = LittleFS.open("/config.json", "w");
  if (!configFile) {
    Serial.println("Failed to open config file for writing");
    return false;
  }

  serializeJson(json, configFile);
  return true;
}


void setOtaFlag(int intOta){
  otaFlag=intOta;
  saveConfig();
  yield();
}

bool clearConfig(){
    Serial.println("DEBUG: In config clear!");
    return LittleFS.format();  
}
