const char* otaServerIndex = "<form method='POST' action='/update' enctype='multipart/form-data'><input type='file' name='update'><input type='submit' value='Update'></form>";

String printDigits(int digits)
{
  String f=String(digits);
  if (digits < 10) return ("0"+f); else return f;
}

// function to calculete Humidex
float calculate_humidex(float temperature, float humidity) {
  float e;
  e = (6.112 * pow(10, (7.5 * temperature / (237.7 + temperature))) * humidity / 100); //vapor pressure
  float humidex = temperature + 0.55555555 * (e - 10.0); //humidex
  return humidex;
}

void handlePageSelect(){
  
   if( server.arg("page").toInt() <=6 ) {
    
      if( server.arg("big") == "1" ) bigDig = true;
      else if (server.arg("big") == "0" ) bigDig = false;
      
      mode0scr = server.arg("page").toInt();
      
      redrawAllScreen();
   }
   
   server.send(200, "text/html", "<script>\nwindow.location.replace(\"http://" + String(myIP) + "\");\n</script>");
}

String genPlot(int *plot_input){
  int max_value = -32000;
  int min_value = 32000;
  String data;
//   String circleData;
//   int point;
  
  for (byte i = 0; i < 15; i++) {
    max_value = max(plot_input[i] , max_value);
    min_value = min(plot_input[i] , min_value);
  }
  if (min_value >= max_value) max_value = min_value + 1;

  data += R"rawliteral( 
  <div class="float">
  <svg viewBox="0 0 460 140" class="chart">
  <line x1="1" x2="500" y1="125" y2="125"  style="stroke:#ccc;stroke-width:1"></line>
  <line x1="35" x2="35" y1="1" y2="140" style="stroke:#ccc;stroke-width:1" ></line>
  )rawliteral";
  data += "<g class=\"labels x-labels\"><text x=\"1\" y=\"10\">" + String(max_value) + "</text>\n<text x=\"1\" y=\"123\">" + String(min_value) + "</text></g>\n";
  
  data += "<polyline fill=\"none\" stroke=\"#0074d9\" stroke-width=\"3\" points=\"";
  
  for (byte i = 0; i < 15; i++) {
    data += " " + String(i*31+35) + "," + String(125-map(plot_input[i], min_value, max_value, 0, 120)) + " ";
//     circleData += "<circle cx=\"" + String(i*31+35) + "\" cy=\"" + String(125-map(plot_input[i], min_value, max_value, 0, 120)) + "\" data-value=\"" + String(plot_input[i]) + "\" r=\"4\"><title>" + String(plot_input[i]) + "</title></circle>\n";
  }
  data += "\"/>\n";
//   data += "<g class=\"plotData\"> \n" + circleData + " </g>\n";
  data += "\"/>\n</svg></div>\n";
  return data;
}

void SetTimeDateAndDate(
                   byte second, // 0-59
                   byte minute, // 0-59
                   byte hour, // 1-23
                   byte dayOfWeek, // 1-7
                   byte dayOfMonth, // 1-28/29/30/31
                   byte month, // 1-12
                   byte year) // 0-99
{ Wire.beginTransmission(DS3231_I2C_ADDRESS);
  Wire.write(0);
  Wire.write(decToBcd(second)); // 0 to bit 7 starts the clock
  Wire.write(decToBcd(minute));
  Wire.write(decToBcd(hour)); // If you want 12 hour am/pm you need to set
  Wire.write(decToBcd(dayOfWeek));
  Wire.write(decToBcd(dayOfMonth));
  Wire.write(decToBcd(month));
  Wire.write(decToBcd(year));
  Wire.endTransmission();}


void HandleClient() 
{
  unsigned long webpageLoad = millis();
  char daysOfTheWeek[12][12] = {"   Sunday", "   Monday", "  Tuesday", "Wednesday", " Thursday", "   Friday", " Saturday"};
  DateTime now = rtc.now();
  String webpage;
  webpage =  "<html>";
  webpage += "<head><title>"+hostName+"</title><meta charset='UTF-8' http-equiv='refresh' content='5' >";
  webpage += "<style>";
  webpage += "body {font-family: Verdana; Color: #00979d;}";
  webpage += "h2 {line-height: 10%;} ";
  webpage += "p {line-height: 20%;} ";
  webpage += ".chart { height: 140px; width: 500; } ";
  webpage += ".chart .grid { stroke: #ccc; stroke-dasharray: 0; stroke-width: 2;} ";
  webpage += ".labels { font-size: 13px;} ";
  webpage += ".chart .plotData { fill: red; stroke-width: 1;} ";
  webpage += ".float { float: left;} ";
  webpage += ".floatclear { clear: both;} ";
  webpage += "</style>";
  webpage += "</head>";
  webpage += "<body>";
  webpage += "<p><b>&nbsp;&nbsp; " + hostName + " </b></p><br>";
  webpage += "<div class=\"floatclear\"></div><br><p>&nbsp; Температура &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;Ощущается как</p>\n";
  webpage += "<h2><br>&nbsp;&nbsp;&nbsp;&nbsp;" + String(dispTemp) + " °C &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"+String(calculate_humidex(dispTemp,dispHum)) + " °C</h2>\n"; 
  webpage += genPlot((int*)tempHour);
  webpage += genPlot((int*)tempDay);   
    
  webpage += "<div class=\"floatclear\"></div><br><p>&nbsp; Влажность </p>\n";
  webpage += "<h2><br>&nbsp;&nbsp;&nbsp;&nbsp;" + String(dispHum) + " %  </h2><br>";

  webpage += genPlot((int*)humHour);
  webpage += genPlot((int*)humDay);  
  
   webpage += "<div class=\"floatclear\"></div><br><p>&nbsp; Атмосферное давление </p>\n";
   webpage += "<h2><br>&nbsp;&nbsp;&nbsp;&nbsp;" + String(dispPres) + " мм р.ст.</h2><br>";

  webpage += genPlot((int*)pressHour);
  webpage += genPlot((int*)pressDay); 
  
  webpage += "<div class=\"floatclear\"></div><br><p>&nbsp; Вероятность осадков </p>\n";
  webpage += "<h2><br>&nbsp;&nbsp;&nbsp;&nbsp;" + String(dispRain) + " %</h2><br>\n";
  
  webpage += "<div class=\"floatclear\"></div>\n<p><br>&nbsp;&nbsp;&nbsp;&nbsp; RTC:" + printDigits(now.hour()) + ":" + printDigits(now.minute()) + ":"  + printDigits(now.second()) + " &nbsp;&nbsp;" + daysOfTheWeek[now.dayOfTheWeek()];
  webpage += " &nbsp;" + printDigits(now.day()) + "/" + printDigits(now.month()) + "/"  + String(now.year()-48) + "</p>&nbsp;&nbsp;\n";

  webpage += "<p>&nbsp;&nbsp;&nbsp;&nbsp; NTP:" + printDigits(ntp.hour()) + ":" + printDigits(ntp.minute()) + ":"  + printDigits(ntp.second()) + " &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;\n";
  webpage += " &nbsp;" + printDigits(ntp.day()) + "/" + printDigits(ntp.month()) + "/"  + String(ntp.year()) + "</p>&nbsp;&nbsp;\n";
   webpage += "<div class=\"floatclear\"></div><br><p>&nbsp; Настройки экрана часов </p>\n";
  webpage += R"rawliteral(   
<form  action="/pageselect" method="get">
  <input type="radio"  name="big" value="0">
  <label>Маленькие цифры</label>
  <input type="radio"  name="big" value="1">
  <label>Большие цифры</label>
  <select name="page">
    <option value="0">Часы</option>
    <option value="2">Температура</option>
    <option value="3">Давление</option>
    <option value="4">Влажность</option>
    <option value="5">Высота</option>
  </select>
  <input type="submit" value="Отправить">
</form>
  )rawliteral";
  webpage += "<p>&nbsp;&nbsp; <a href='http://"+String(myIP)+"/set_WI_FI'>НАСТРОЙКИ</a></p><BR>";
  webpage += "<p>&nbsp;&nbsp; <a href='http://"+String(myIP)+"/set_clock'>КОРРЕКЦИЯ ДАТЫ И ВРЕМЕНИ</a></p><BR>";
  webpage += "<center><p>&nbsp;&nbsp;&nbsp;&nbsp;Время работы: "+Uptime()+",&nbsp;&nbsp;WiFi сигнал: "+ String( WiFi.RSSI()) + "dBm, </p><p>&nbsp;&nbsp;&nbsp;&nbsp;Время генерации страницы: "+String(millis()-webpageLoad)+"msec</p><br></center><br>\n";
  webpage += "</body>";
  webpage += "</html>";
  server.send(200, "text/html", webpage);
}

void SetTime() {
  DateTime now = rtc.now();
  int nhour =  now.hour();
  int shour = nhour;
  int nminute = now.minute();
  int sminute = nminute;
  int ndayOfTheWeek = now.dayOfTheWeek();
  int sdayOfTheWeek = ndayOfTheWeek;
  int nday = now.day();
  int sday = nday;
  int nmonth = now.month();
  int smonth = nmonth;
  int nyear = now.year();
  int syear = nyear - 48;
  int tmp_arg = 0;
  
  String webpage;
  webpage =  "<html>"; 
  webpage += "<head><title>ESP8266 clock setup </title><meta charset='UTF-8'>";
  webpage += "<style>"; 
  webpage += "body { font-family: Verdana; Color: #00979d;}";
  webpage += "</style>";
  webpage += "</head>"; 
  webpage += "<body>";
  
 
  webpage += "<p><b>&nbsp;&nbsp;- Настройка часов -</b></p><BR>";
 webpage += "<form action='http://"+String(myIP)+"/set_clock' method='POST'>";  
  webpage += "<p>&nbsp;&nbsp;&nbsp;Настройка времени и даты</p>";
  webpage += "&nbsp;&nbsp;&nbsp; Часы: <input type='text' name='input_HH' placeholder='" + String(nhour) + "'>";
  webpage += "&nbsp; Минуты: <input type='text' name='input_MI' placeholder='" + String(nminute) + "'><BR><BR>";
  webpage += "&nbsp;&nbsp;&nbsp; День недели: <input type='text' name='input_DW' placeholder='" + String(ndayOfTheWeek) + "'><BR><BR>";
  webpage += "&nbsp;&nbsp;&nbsp; День: <input type='text' name='input_DM' placeholder='" + String(nday) + "'>";
  webpage += "&nbsp;&nbsp; Месяц: <input type='text' name='input_MO' placeholder='" + String(nmonth) + "'><BR><BR>";
  webpage += "&nbsp; Год: <input type='text' name='input_YE' placeholder='" + String(nyear) + "'><BR><BR>&nbsp;&nbsp;&nbsp;&nbsp;<input type='submit' value='Ввод'>";
  webpage += "<p>&nbsp;&nbsp; <a href='http://"+String(myIP)+"/set_WI_FI'>НАСТРОЙКИ</a></p>";
  webpage += "<p>&nbsp;&nbsp; <a href='http://"+String(myIP)+"/'>ГЛАВНОЕ МЕНЮ</a></p>";
  webpage += "</form>";
  webpage += "</body>";
  webpage += "</html>";
  server.send(200, "text/html", webpage); // Send a response to the client asking for input
  if (server.args() > 0 ) { // Arguments were received
    for ( uint8_t i = 0; i < server.args(); i++ ) {
  
      if (server.argName(i) == "input_HH")
      {

        tmp_arg = server.arg(i).toInt();
        if (tmp_arg < 25) {
          shour = tmp_arg;
        }
      }

      if (server.argName(i) == "input_MI")  {
   
        tmp_arg = server.arg(i).toInt();
        if (tmp_arg < 60) {
          sminute = tmp_arg;
        }
      }

      if (server.argName(i) == "input_DW")  {

        tmp_arg = server.arg(i).toInt();
        if ((tmp_arg > 0) && (tmp_arg < 8)) {
          sdayOfTheWeek = tmp_arg;
        }
      }

      if (server.argName(i) == "input_DM") {

        tmp_arg = server.arg(i).toInt();
        if ((tmp_arg > 0) && (tmp_arg < 32)) {
          sday = tmp_arg;
        }
      }

      if (server.argName(i) == "input_MO") {

        tmp_arg = server.arg(i).toInt();
        if ((tmp_arg > 0) && (tmp_arg < 13)) {
          smonth = tmp_arg;
        }
      }

      if (server.argName(i) == "input_YE") {

        tmp_arg = server.arg(i).toInt();
        if ((tmp_arg > 2000) && (tmp_arg < 2100)) {
          syear = tmp_arg - 48;
        }
      }

    }

    SetTimeDateAndDate(00, sminute, shour, sdayOfTheWeek, sday, smonth, syear);
   redrawAllScreen();

  }
}

void handleRoot() {
    String webpage;
  webpage =  "<html>"; 
  webpage += "<head><title> Setup </title><meta charset='UTF-8'>";
  webpage += "<style>";
  webpage += "body { font-family: Verdana; Color: #00979d;}";
  webpage += "</style>";
  webpage += "</head>";
  webpage += "<body>";

  String str = "";
  str += webpage;
  str += "<body>\
   <form method=\"POST\" action=\"ok\">\
     <input type=\"radio\" value=\"1\" name=\"otaflag\"> Загрузить новую прошивку (после перезагрузки зайдите на страницу устройства и запустите процедуру)</br></br>\
     <input type=\"text\" value=\"" + ssid + "\" name=\"ssid\" maxlength=32> WiFi SSID</br></br>\
     <input type=\"password\" value=\"" + pass + "\" name=\"pswd\" maxlength=64> PASSWORD</br></br>\
     <input type=\"text\" value=\"" + TIMEZONE + "\" name=\"tzn\" maxlength=3> TIMEZONE</br></br>\
     <input type=\"text\" value=\"" + mqtt_ip + "\" name=\"mqtt_ip\" maxlength=15> MQTT IP</br></br>\
     <input type=\"text\" value=\"" + mqtt_port + "\" name=\"mqtt_port\" maxlength=5> MQTT PORT</br></br>\
     <input type=\"text\" value=\"" + mqtt_auth + "\" name=\"mqtt_auth\" maxlength=32> MQTT USER</br></br>\
     <input type=\"password\" value=\"" + mqtt_pass + "\" name=\"mqtt_pass\" maxlength=32> MQTT PWD</br></br>\
     <input type=\"text\" value=\"" + mqtt_Temp + "\" name=\"mqtt_temp\" maxlength=64> MQTT Topic (temperature)</br></br>\
     <input type=\"text\" value=\"" + mqtt_Hum + "\" name=\"mqtt_hum\" maxlength=64> MQTT Topic (humidity)</br></br>\
     <input type=\"text\" value=\"" + mqtt_Press + "\" name=\"mqtt_press\" maxlength=64> MQTT Topic (pressure)</br></br>\
     <input type=SUBMIT value=\"Save\">\
   </form>\
 </body>\
</html>";

 server.send ( 200, "text/html", str );
} 

void handleOk(){
  String webpage;
  webpage =  "<html>";
  webpage += "<head><title>settings save </title><meta charset='UTF-8'>";
  webpage += "<style>";
  webpage += "body { font-family: Verdana; Color: #00979d;}";
  webpage += "</style>";
  webpage += "</head>"; 
  webpage += "<body>";
  String ssid_ap       = server.arg("ssid");
  String pass_ap       = server.arg("pswd");
  String TZN_ap        = server.arg("tzn");
  String mqtt_ip_ap    = server.arg("mqtt_ip");
  String mqtt_port_ap  = server.arg("mqtt_port");
  String mqtt_auth_ap  = server.arg("mqtt_auth");
  String mqtt_pass_ap  = server.arg("mqtt_pass");
  String mqtt_temp_ap  = server.arg("mqtt_temp");
  String mqtt_hum_ap   = server.arg("mqtt_hum");
  String mqtt_press_ap = server.arg("mqtt_press");
  String otaFlag_ap    = server.arg("otaflag");
  int tz;
  String str = "";
 
  str += webpage;
  str += "<body>";
  tz = TZN_ap.toInt();

  (otaFlag_ap == "0") ? otaFlag = 0 : otaFlag = 1;
  
  if( (tz > -12) && (tz < 12) ) TIMEZONE = tz;
  mqtt_ip    = mqtt_ip_ap;
  mqtt_port  = mqtt_port_ap;
  
  mqtt_auth_ap.replace("%2F","/");
  mqtt_auth  = mqtt_auth_ap;
  
  mqtt_pass_ap.replace("%2F","/");
  mqtt_pass  = mqtt_pass_ap;
  
  mqtt_temp_ap.replace("%2F","/");
  mqtt_Temp  = mqtt_temp_ap;
  
  mqtt_hum_ap.replace("%2F","/");
  mqtt_Hum   = mqtt_hum_ap;
  
  mqtt_press_ap.replace("%2F","/");
  mqtt_Press = mqtt_press_ap;
  
  ssid_ap.replace("%2F","/");
  pass_ap.replace("%2F","/");

  str +="Configuration saved in FS</br>\<a href=\"/\">Return</a> to settings page</br>";
  str += "</body></html>";
  server.send ( 200, "text/html", str );
  
  saveConfig();

  if( (ssid_ap != String(ssid)) || (pass_ap != String(pass)) ){
      ssid = ssid_ap;
      pass = pass_ap;
      saveConfig();
      delay(1000);
      ESP.restart();
      delay(100);
  }
  if( otaFlag ) {
    lcd.clear();
    lcd.print("Rebooting...");
    delay(1000);
    ESP.restart();
    delay(100);
  }
}

void handleOTA() {
      Serial.println("Starting OTA mode.");    
      Serial.printf("Sketch size: %u\n", ESP.getSketchSize());
      Serial.printf("Free size: %u\n", ESP.getFreeSketchSpace());
      MDNS.begin(host);
      server.on("/", HTTP_GET, [](){
        server.sendHeader("Connection", "close");
        server.sendHeader("Access-Control-Allow-Origin", "*");
        server.send(200, "text/html", otaServerIndex);
      });
      server.on("/update", HTTP_POST, [](){
        server.sendHeader("Connection", "close");
        server.sendHeader("Access-Control-Allow-Origin", "*");
        server.send(200, "text/plain", (Update.hasError())?"FAIL":"OK");
        setOtaFlag(0); 
        lcd.clear();
        delay(100);
        ESP.restart();
      },[](){
        HTTPUpload& upload = server.upload();
        if(upload.status == UPLOAD_FILE_START){
          //Serial.setDebugOutput(true);
          WiFiUDP::stopAll();
          Serial.printf("Update: %s\n", upload.filename.c_str());
          otaCount=300;
          uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
          if(!Update.begin(maxSketchSpace)){//start with max available size
            Update.printError(Serial);
          }
        } else if(upload.status == UPLOAD_FILE_WRITE){
          if(Update.write(upload.buf, upload.currentSize) != upload.currentSize){
            Update.printError(Serial);
          }
        } else if(upload.status == UPLOAD_FILE_END){
          if(Update.end(true)){ //true to set the size to the current progress
            Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
          } else {
            Update.printError(Serial);
          }
          Serial.setDebugOutput(false);
        }
        yield();
      });
      server.begin();
      Serial.printf("Ready! Open http://%s.local in your browser\n", host);
      MDNS.addService("http", "tcp", 80);
      otaTickLoop.attach(1, otaCountown);
}
