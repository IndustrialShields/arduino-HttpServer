// HttpServer library example
// by Industrial Shields

#ifdef MDUINO_PLUS
#include <Ethernet2.h>
#else
#include <Ethernet.h>
#endif

#include <HttpServer.h>

HttpServer http;

////////////////////////////////////////////////////////////////////////////////////////////////////
void setup() {
  Serial.begin(9600UL);
  Serial.println("HTTP server started");

  byte mac[] = {0xAF, 0xBE, 0xCD, 0xDC, 0xEB, 0xFA};
  IPAddress ip(10, 10, 10, 16);
  Ethernet.begin(mac, ip);

  http.begin();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void loop() {
  http.update();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
void httpServerEvent(const HttpRequest &req, HttpResponse &res) {
  Serial.print("client: ");
  Serial.println(req.remoteIP);
  Serial.print("method: ");
  Serial.println(req.method);
  Serial.print("route: ");
  Serial.println(req.route);
  Serial.print("query string: ");
  Serial.println(req.queryString);
  Serial.print("body: ");
  Serial.println(req.body);

  if (req.route == "/") {
    // Send HTML content
    res.html("<h1>HTTP server test</h1>");

  } else if (req.route == "/test") {
    String content;
    content += "millis: " + String(millis()) + "\n";
    content += "method: " + req.method + "\n";
    content += "route: " + req.route + "\n";
    content += "query string: " + req.queryString + "\n";
    content += "abc: " + req.queryString.getValue(F("abc")) + "\n";
    // Send text content
    res.text(content);

  } else if (req.route == "/info") {
    // Redirect to another route
    res.redirect("/test?" + req.queryString);

  } else {
    // Send an error
    res.send("Not Found", "text/plain", 404, F("Not Found"));
  }
}
