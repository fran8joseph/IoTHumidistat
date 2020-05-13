#include <LEAmDNS.h>
#include <LEAmDNS_Priv.h>
#include <ESP8266mDNS.h>
#include <LEAmDNS_lwIPdefs.h>
#include <ESP8266mDNS_Legacy.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <ESP8266WiFiMulti.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>

//Headers for DTH11

#include <dht.h>
#include <SPI.h>
#include <Wire.h>

#include <Servo.h>

ESP8266WiFiMulti WiFiMulti;
ESP8266WebServer server(80);
dht DHT;
Servo servo; 

int humidity;
float temperature;

float ctemp;
float cfeels;
float cminTemp;
float cmaxTemp;
int cpressure;
int chumidity;
const char* howIsTheSky;

float wind_speed; // 1.79
int wind_deg; // 253
float wind_gust;

 int loopCounter=0;
 bool onStartup = false;
  
void handleRoot();              // function prototypes for HTTP handlers
void handleLogin();
void handleWebData();
String handleTimeData();
String getWeatherInfoAPI();
String getDateAndTimeAPI();
void handleNotFound();
float getRoomTemperature();
int getRoomHumidity();
String SendHTML(float, int, String);
float getOutsideTemperature();
int getOutsideHumid();
void setOptimumHumidity(float);
String setWeatherForecast(float,float,float,int,int,const char*,float,int,float);
void controlServoAngle(int);


void setup()
{
  Serial.begin(9600);
  servo.attach(0);
 // Serial.println(" In set up : reset servo");
 servo.write(130);
 delay(1000);

 onStartup = true;
 
  int chk = DHT.read11(D1);

  WiFi.mode(WIFI_STA);

  WiFiMulti.addAP("SSID", "pwd");
  WiFi.begin("SSID", "pwd");
  Serial.print("Connecting");

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(2000);
    Serial.print(".");
  }
  Serial.println();

  Serial.print("Connected, IP address: ");
  Serial.println(WiFi.localIP());

  delay(3000);

  if (MDNS.begin("esp8266")) {              // Start the mDNS responder for esp8266.local
    Serial.println("mDNS responder started");
  } else {
    Serial.println("Error setting up MDNS responder!");
  }

  server.on("/", HTTP_GET, handleRoot);
  server.on("/login", HTTP_POST, handleLogin);
  server.on("/list", HTTP_GET, handleWebData);
  server.onNotFound(handleNotFound);

  server.begin(); // Actually start the server
  Serial.println("HTTP server started");

  delay(3000);

}

void loop() {

  server.handleClient();

  if(loopCounter == 6500 || onStartup==true){
    getRoomTemperature();
    getRoomHumidity();
    handleWebData();
    // Serial.print("from loop :");
    // Serial.println(onStartup);
    loopCounter=0;
    onStartup=false;
  }
  loopCounter+=1;
 
  //Serial.println(loopCounter);
 delay(50);
}

void controlServoAngle(int optVal){
  servo.attach(D4);
  delay(150);

  int requiredHumid = optVal;
  int currentHumid = getRoomHumidity();
  
  if(currentHumid <= (requiredHumid+2)){
        servo.write(10);
        delay(50);
  }
  else if(currentHumid >= (requiredHumid+2)){
  
        servo.write(130);
        delay(50);
        
  }
}

float getRoomTemperature() {
  float temperature = DHT.temperature;
  return temperature;
}

int getRoomHumidity() {
  int humidity = DHT.humidity;
  return humidity;
}

    
void setOptimumHumidity(float _outT){
  //float _roomT=getRoomTemperature();
  int _roomH =getRoomHumidity();
  int optRoomHumidity;

  if(_outT < -28){
    optRoomHumidity = 15;
  }
  else if(( _outT > -28) && (_outT < -23)){
    optRoomHumidity = 20;
  }
  else if(( _outT > -23) && (_outT < -18)){
    optRoomHumidity = 25;
  }
  else if(( _outT > -18) && (_outT < -12)){
    optRoomHumidity = 30;
  }
  else if(( _outT > -12) && (_outT < -7)){
    optRoomHumidity = 35;
  }
  else if(( _outT > -7) && (_outT < 10)){
    optRoomHumidity = 40;
  }
  else if(( _outT > 10) && (_outT < -23)){
    optRoomHumidity = 45;
  }
  Serial.print("Temp out:");
  Serial.println(_outT);
  
   Serial.println("Optimum Humidity:");
   Serial.print(optRoomHumidity);

   controlServoAngle(optRoomHumidity);
   
  //return optRoomHumidity;
}

String getResponseFromAPI(String endPoint) {

  if (WiFiMulti.run() == WL_CONNECTED)
  {
    WiFiClient client;
    HTTPClient http;

    // http.setReuse(true);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Accept", "*/*");
    http.addHeader("Accept-Encoding", "gzip, deflate, br");
    http.begin(client, endPoint);  //"http://api.openweathermap.org/data/2.5/weather?id=6066513&&units=metric&appid=?");
    int httpCode = http.GET();

   // Serial.println("httpCode:");
   // Serial.print(httpCode);

    if (httpCode > 0)
    {
      //Serial.println("start to serialize");

      String response = http.getString();

      return response;
    }
    else {
      Serial.printf("HTTP GET failed, error: %s\n", http.errorToString(httpCode).c_str());
      http.end();
      return "Error";
    }
  } else {
    Serial.println("WiFi Conncetion Lost");
  }
}


void handleRoot() {                          // When URI / is requested, send a web page with a button to toggle the LED
  server.send(200, "text/html", "<form action=\"/login\" method=\"POST\"><input type=\"text\" name=\"username\" placeholder=\"Username\"></br><input type=\"password\" name=\"password\" placeholder=\"Password\"></br><input type=\"submit\" value=\"Login\"></form><p>Try 'John Doe' and 'password123' ...</p>");
}

void handleLogin() {                         // If a POST request is made to URI /login
  if ( ! server.hasArg("username") || ! server.hasArg("password")
       || server.arg("username") == NULL || server.arg("password") == NULL) { // If the POST request doesn't have username and password data
    server.send(400, "text/plain", "400: Invalid Request");         // The request is invalid, so send HTTP status 400
    return;
  }
  if (server.arg("username") == "John Doe" && server.arg("password") == "password123") { // If both the username and the password are correct
    //server.send(200, "text/html", "<h1>Welcome, " + server.arg("username") + "!</h1><p>Login successful</p>");
    server.send(200, "text/html", "<h1>Welcome, " + server.arg("username") + "!</h1><p>Login successful</p> <br></br> <form action=\"/list\" method=\"GET\"></br><input type=\"submit\" value=\"List Data\"></form>");
  }
  else
  { // Username and password don't match
    server.send(401, "text/plain", "401: Unauthorized");
  }
}


void handleWebData() {

  String openWeatherAPI = "http://api.openweathermap.org/data/2.5/weather?id=6066513&&units=metric&appid=c8de7f5ac681549b72372579b0c8efcc";
  String responseData = getResponseFromAPI(openWeatherAPI);  //Get weather data from Web

  DynamicJsonDocument root(2048);

  DeserializationError error = deserializeJson(root, responseData);

  if (error) {
    Serial.print("openWeatherAPI deserializeJson() failed with code ");
    Serial.println(error.c_str());
  }
  else {
    JsonObject tempInfo = root["main"];
    ctemp = tempInfo["temp"];
    cfeels = tempInfo["feels_like"];
    cminTemp = tempInfo["temp_min"];
    cmaxTemp = tempInfo["temp_max"];
    cpressure = tempInfo["pressure"];
    chumidity = tempInfo["humidity"];
    // getRoomData();
    //float roomTemp =getRoomTemperature();
    //int roomHumid = getRoomHumidity();
    setOptimumHumidity(ctemp);

    //Gather Forecast & Weather info

    JsonObject forecastInfo = root["weather"][0];

    howIsTheSky = forecastInfo["description"];

    JsonObject windInfo=root["wind"];
    
    wind_speed = windInfo["speed"]; // 1.79
    wind_deg = windInfo["deg"]; // 253
    wind_gust = windInfo["gust"];

    String displayForecast=setWeatherForecast(cfeels,cminTemp,cmaxTemp,cpressure,chumidity,howIsTheSky,wind_speed,wind_deg,wind_gust);
    
    server.send(200, "text/html", SendHTML(ctemp, chumidity,displayForecast));
  }
  
}
                          
String  setWeatherForecast(float fc_feelsLike,float fc_minTemp,float fc_maxTemp,int fc_pressure,int fc_humidity, const char* fc_howIsTheSky,float fc_windSpeed, int fc_windDeg, float fc_windGust){

  String weatherForecast;
weatherForecast+=        "<table id=\"forecast\" style=\"width:50%\">";
weatherForecast+=         " <tr> ";
weatherForecast+=           " <h3>Forecast</h3> ";
weatherForecast+=          "</tr>";
//weatherForecast+= "<td colspan=\"3\"style=\"text-align: center;\"><p>";
//weatherForecast+= "Here is the weather forecast for the day</p></td>"; 
weatherForecast+=            "<tr><td class=\"forecastTable\">";
weatherForecast+=             " Feels like : <span>";
weatherForecast+= fc_feelsLike;
weatherForecast+="C </span></td>";
weatherForecast+="            <td class=\"forecastTable\">";
weatherForecast+="              Min Temp : <span>";
weatherForecast+=fc_minTemp;
weatherForecast+="C </span>";
weatherForecast+="            </td>";
weatherForecast+="            <td class=\"forecastTable\">";
weatherForecast+="              Max Temp :<span>";
weatherForecast+= fc_maxTemp;
weatherForecast+= "C</span>";
weatherForecast+= "           </td></tr>";
weatherForecast+= "        <tr>   <td class=\"forecastTable\">";
weatherForecast+="              Pressure: <span>";
weatherForecast+=fc_pressure;
weatherForecast+="bar  </span></td>";
weatherForecast+="          <td colspan=\"3\" style=\"text-align: left;\">How is Sky :";
weatherForecast+=fc_howIsTheSky;
weatherForecast+="         </td> </tr> <tr><td class=\"forecastTable\"> Wind Speed : ";
weatherForecast+=fc_windSpeed;
weatherForecast+= " </td> <td class=\"forecastTable\">Wind Degree :";
weatherForecast+= fc_windDeg;
weatherForecast+=" </td> <td class=\"forecastTable\"> Wind Gust : ";
weatherForecast+=fc_windGust;
weatherForecast+="</tr></td> </table>";

//  Serial.println(" Weather info: ");
//  Serial.print(weatherForecast);

  return weatherForecast;
}
    

String handleTimeData() {

  String worldClockAPI = "http://worldclockapi.com/api/json/est/now";
  String timeResponseData = getResponseFromAPI(worldClockAPI);  //Get weather data from Web

  DynamicJsonDocument timeRoot(1024);

  DeserializationError t_error = deserializeJson(timeRoot, timeResponseData);

  if (t_error) {
    Serial.print("t_error deserializeJson() failed with code ");
    Serial.println(t_error.c_str());
    return "error";
  }
  else {

    String currentDateTime = timeRoot["currentDateTime"]; // "2020-05-05T23:09-04:00"

    String dayOfTheWeek = timeRoot["dayOfTheWeek"]; // "Tuesday"

    return  currentDateTime;
  }

}

String SendHTML(float Temperaturestat, int Humiditystat, String foreCast) {
  
  loopCounter=0;

  Serial.print("from Page refresh");
  
  String ptr = "<!DOCTYPE html> <html>\n";
  "<!DOCTYPE html> <html>\n";
  ptr += "<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
  ptr += "<link href=\"https://fonts.googleapis.com/css?family=Open+Sans:300,400,600\" rel=\"stylesheet\">\n";
  ptr += "<title>Markham Weather Report</title>\n";
  ptr += "<style>html { font-family: 'Open Sans', sans-serif; display: block; margin: 0px auto; text-align: center;color: #333333;}\n";
  ptr += "body{margin-top: 50px;}\n";
  ptr += "h1 {margin: 50px auto 30px;}\n";
  ptr += "h4 {margin: 50px auto 30px;font-size: 15px;}\n";
  ptr += "table{  border:1px dotted black;margin-left:auto;margin-right:auto;padding:2px; border-spacing:20px; }";
  ptr += "tr,td {text-align:center;}";
  ptr += ".side-by-side{display: inline-block;vertical-align: middle;position: relative;}\n";
  ptr += ".humidity-icon{background-color: #3498db;width: 20px;height: 30px;border-radius: 50%;line-height: 36px;}\n";
  ptr += ".humidity-text{font-weight: 600;padding-left:5px;font-size: 19px;width: 100px;text-align: left;}\n";
  ptr += ".humidity{font-weight: 300;font-size: 30px;color: #3498db;}\n";
  ptr += ".temperature-icon{background-color: #f39c12;width: 20px;height: 30px;border-radius: 50%;line-height: 40px;}\n";
  ptr += ".temperature-text{font-weight: 600;padding-left:5px;font-size: 19px;width: 100px;text-align: left;}\n";
  ptr += ".temperature{font-weight: 300;font-size: 30px;color: #f39c12; padding:5px}\n";
  ptr += ".superscript{font-size: 15px;font-weight: 600;position: absolute;top: 10px;}\n";
  ptr += ".data{padding: 10px;}\n";
  ptr += ".tableHeader{padding: 5px;width:145px;text-align: center;} \n .forecastTable{width: 15px; text-align: left;}";
  ptr += "</style>\n";
  ptr += "<script>\n";
  ptr += "   setInterval(loadWeatherData,60000) \n";
  ptr += "   function loadWeatherData() {\n";
  ptr += "      var xhttp = new XMLHttpRequest() \n";
  ptr += "    xhttp.onreadystatechange = function() {\n";
  ptr += "    if (this.readyState == 4 && this.status == 200) {\n";
  ptr += "      document.getElementById(\"webpage\").innerHTML =this.responseText \n";
  ptr += " } \n";
  ptr += "}\n";
  ptr += " xhttp.open(\"GET\", \"/list\", true);\n";
  ptr += " xhttp.send(); \n";
  ptr += " } \n";
  ptr += " </script>\n";
  ptr += " </head>\n";
  ptr += " <body>\n";
  ptr += "   <div id=\"webpage\">\n";
  ptr += "   <h2>Markham Weather Report</h2>\n";
  ptr += "   <table id=\"t_fromWeb\" style=\"width:50%\">\n";
  ptr += "       <tr>\n";
  ptr += "          <th></th>\n";
  ptr += "           <th></th>\n";
  ptr += "           <th class=\"tableHeader\">Markham</th>\n";
  ptr += "          <th class=\"tableHeader\">Room</th>\n";
  ptr += "     </tr>\n";
  ptr += "      <tr>\n";
  ptr += "           <td>\n";
  ptr += "               <div class=\"side-by-side temperature-icon\">\n";
  ptr += "                   <svg version=\"1.1\" id=\"Layer_1\" xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" x=\"0px\" y=\"0px\"\n";
  ptr += "                            width=\"9.915px\" height=\"22px\" viewBox=\"0 0 9.915 22\" enable-background=\"new 0 0 9.915 22\" xml:space=\"preserve\">\n";
  ptr += "                            <path fill=\"#FFFFFF\" d=\"M3.498,0.53c0.377-0.331,0.877-0.501,1.374-0.527C5.697-0.04,6.522,0.421,6.924,1.142 \n";
  ptr += "                           c0.237,0.399,0.315,0.871,0.311,1.33C7.229,5.856,7.245,9.24,7.227,12.625c1.019,0.539,1.855,1.424,2.301,2.491 \n";
  ptr += "                           c0.491,1.163,0.518,2.514,0.062,3.693c-0.414,1.102-1.24,2.038-2.276,2.594c-1.056,0.583-2.331,0.743-3.501,0.463\n";
  ptr += "                          c-1.417-0.323-2.659-1.314-3.3-2.617C0.014,18.26-0.115,17.104,0.1,16.022c0.296-1.443,1.274-2.717,2.58-3.394\n";
  ptr += "                          c0.013-3.44,0-6.881,0.007-10.322C2.674,1.634,2.974,0.955,3.498,0.53z\"/>\n";
  ptr += "                  </svg>\n";
  ptr += "               </div>\n";
  ptr += "           </td>\n";
  ptr += "           <td>\n";
  ptr += "               <div class=\"side-by-side temperature-text\">Temperature</div>\n";
  ptr += "           </td>\n";
  ptr += "          <td>\n";
  ptr += "             <div class=\"side-by-side temperature\">";
  ptr += Temperaturestat;
  ptr += "<span class=\"superscript\"> °C </span></div>\n";
  ptr += "           </td>\n";
  ptr += "           <td>\n";
  ptr += "               <div id=\"deg_room\"  class=\"side-by-side temperature\">" ;
  ptr += getRoomTemperature() ;
  ptr += "<span class=\"superscript\"> °C </span></div>\n";
  ptr += "           </td>\n";
  ptr += "        </tr>\n";
  ptr += "      <tr>\n";
  ptr += "           <td>\n";
  ptr += "               <div class=\"side-by-side humidity-icon\">\n";
  ptr += "                    <svg version=\"1.1\" id=\"Layer_2\" xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" x=\"0px\" y=\"0px\" width=\"12px\" height=\"17.955px\" viewBox=\"0 0 13 17.955\" enable-background=\"new 0 0 13 17.955\" xml:space=\"preserve\">\n";
  ptr += "                   <path fill=\"#FFFFFF\" d=\"M1.819,6.217C3.139,4.064,6.5,0,6.5,0s3.363,4.064,4.681,6.217c1.793,2.926,2.133,5.05,1.571,7.057 \n";
  ptr += "                   c-0.438,1.574-2.264,4.681-6.252,4.681c-3.988,0-5.813-3.107-6.252-4.681C-0.313,11.267,0.026,9.143,1.819,6.217\"></path>\n";
  ptr += "                   </svg>\n";
  ptr += "               </div>\n";
  ptr += "           </td>\n";
  ptr += "           <td>\n";
  ptr += "              <div class=\"side-by-side humidity-text\">Humidity</div>\n";
  ptr += "            </td>\n";
  ptr += "            <td>\n";
  ptr += "               <div class=\"side-by-side humidity\">\n";
  ptr += Humiditystat;
  ptr += "<span class=\"superscript\">%</span></div>\n";
  ptr += "           </td>\n";
  ptr += "           <td>\n";
  ptr += "               <div id=\"deg_room\" class=\"side-by-side humidity\">\n";
  ptr += getRoomHumidity();
  ptr += "<span class=\"superscript\">%</span></div>\n";
  ptr += "           </td>\n";
  ptr += "        </div>\n";
  ptr += "       </tr>\n";
  ptr += "   </table>\n";
  ptr+=foreCast;
  //add forecast table here
  ptr += "<h4> Observed at:   ";
  ptr += handleTimeData();
  ptr += " </h4>\n";
  ptr += "</div>\n";
  ptr += " </body>\n";
  ptr += "  </html>\n";
  return ptr;
}


void handleNotFound()
{
  server.send(404, "text/html", "404: Not found latest"); // Send HTTP status 404 (Not Found) when there's no handler for the URI in the request
}
