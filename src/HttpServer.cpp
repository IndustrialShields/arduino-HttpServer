/*
   Copyright (c) 2018 Boot&Work Corp., S.L. All rights reserved
   This library is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.
   You should have received a copy of the GNU Lesser General Public License
   along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "HttpServer.h"

__attribute__((weak)) void httpServerEvent(const HttpRequest &req, HttpResponse &res);

#if defined(ESCAPE_FORM_STRING_VALUES)
const struct formStringEscape {
  char code[4];
  char value[2];
} formStringEscapes[] = {
  { "%0A", "\n" },
  { "%0D", "\r" },
  { "%20", " " },
  { "%21", "!" },
  { "%22", "\"" },
  { "%23", "#" },
  { "%24", "$" },
  { "%25", "%" },
  { "%26", "&" },
  { "%27", "'" },
  { "%28", "(" },
  { "%29", ")" },
  { "%2A", "*" },
  { "%2B", "+" },
  { "%2C", "," },
  { "%2D", "-" },
  { "%2E", "." },
  { "%2F", "/" },
  { "%3A", ":" },
  { "%3B", ";" },
  { "%3C", "<" },
  { "%3D", "=" },
  { "%3E", ">" },
  { "%3F", "?" },
  { "%40", "@" },
  { "%5B", "[" },
  { "%5C", "\\" },
  { "%5D", "]" },
  { "%5E", "^" },
  { "%5F", "_" },
  { "%60", "`" },
  { "%7B", "{" },
  { "%7C", "|" },
  { "%7D", "}" },
  { "%7E", "~" },
};

////////////////////////////////////////////////////////////////////////////////////////////////////
static void escape(String &str) {
  const struct formStringEscape *ptr = formStringEscapes;
  for (unsigned i = 0; i < sizeof(formStringEscapes) / sizeof(struct formStringEscape); ++i) {
    str.replace(ptr->code, ptr->value);
    ++ptr;
  }
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////
FormString::FormString(): String() {

}

////////////////////////////////////////////////////////////////////////////////////////////////////
FormString::FormString(const String &str) : String(str) {

}

////////////////////////////////////////////////////////////////////////////////////////////////////
String FormString::getValue(const String &name) const {
  int paramPosition = 0;

  if (!startsWith(name + '=')) {
    paramPosition = indexOf("&" + name + '=');
    if (paramPosition < 0) {
      return "";
    }
    ++paramPosition;
  }

  int valuePosition = paramPosition + name.length() + 1;
  String ret = substring(valuePosition, indexOf('&', valuePosition));
#if defined ESCAPE_FORM_STRING_VALUES
  escape(ret);
#endif

  return ret;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
HttpResponse::HttpResponse(EthernetClient &client) : _client(client) {

}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool HttpResponse::send(const String &body, const String &contentType, uint16_t status, const String &statusText) {
  if (!_client.connected()) {
    return false;
  }

  // Headers
  _client.print(F("HTTP/1.1 "));
  _client.print(status);
  _client.print(' ');
  _client.println(statusText);
  // Content type
  _client.print(F("Content-Type: "));
  _client.println(contentType);
  // Content length
  _client.print(F("Content-Length: "));
  _client.println(body.length());

  // Blank line
  _client.println();
  // Body
  _client.println(body);

  // Wait to be sent
  _client.flush();

  return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool HttpResponse::send(const __FlashStringHelper *body, const __FlashStringHelper *contentType, uint16_t status, const __FlashStringHelper *statusText) {
  if (!_client.connected()) {
    return false;
  }

  // Headers
  _client.print(F("HTTP/1.1 "));
  _client.print(status);
  _client.print(' ');
  _client.println(statusText);
  // Content type
  _client.print(F("Content-Type: "));
  _client.println(contentType);
  // Content length
  _client.print(F("Content-Length: "));
  PGM_P p = reinterpret_cast<PGM_P>(body);
  size_t n = 0;
  while (1) {
    unsigned char c = pgm_read_byte(p++);
    if (c == 0) break;
    n++;
  }
  _client.println(n);

  // Blank line
  _client.println();
  // Body
  _client.println(body);

  // Wait to be sent
  _client.flush();

  return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool HttpResponse::sendStream(Stream &stream, const String &contentType){
  uint8_t buff[512];
  uint16_t counter = 0;
  uint16_t buff_len = 512;

  if (!_client.connected()) {
    return false;
  }

  // Headers
  _client.print(F("HTTP/1.1 200 OK"));
  // Content type
  _client.print(F("Content-Type: "));
  _client.println(contentType);
  // Content length
  _client.print(F("Content-Length: "));
  _client.println(stream.available());

  // Blank line
  _client.println();
  // Body
  while (stream.available()){
    buff[counter++] = stream.read();
    if (counter >= buff_len){
      _client.write(buff, counter);
      //stream.write(buff, counter);
      counter = 0;
    }
  }
  if (counter > 0) {
    _client.write(buff, counter);
    //stream.write(buff, counter);
  }
  // Wait to be sent
  _client.flush();

  return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool HttpResponse::redirect(const String &dest) {
  if (!_client.connected()) {
    return false;
  }

  // Headers
  _client.println(F("HTTP/1.1 303 See Other"));
  // Content type
  _client.print(F("Location: "));
  _client.println(dest);
  _client.println();

  return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool HttpResponse::redirect(const __FlashStringHelper *dest) {
  if (!_client.connected()) {
    return false;
  }

  // Headers
  _client.println(F("HTTP/1.1 303 See Other"));
  // Content type
  _client.print(F("Location: "));
  _client.println(dest);
  _client.println();

  return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
HttpServer::HttpServer(uint16_t port) : EthernetServer(port) {

}

////////////////////////////////////////////////////////////////////////////////////////////////////
void HttpServer::update() {
  EthernetClient client = available();
  if (client) {
    size_t contentLength = 0;
    int section = MethodSection;
    String headerName;
    String headerValue;

    // Create the request and response objects
    HttpRequest req;
    HttpResponse res(client);

    // Parse the request
    // It is finished when all the data is received or when the connection is closed
    while (client.connected() && (section < FinishedSection)) {
      if (client.available()) {
        char c = client.read();

        switch (section) {
          case MethodSection:
            if (c == ' ') {
              section = PathSection;
            } else {
              req.method += c;
            }
            break;

          case PathSection:
            if (c == ' ') {
              section = VersionSection;
            } else if (c == '?') {
              section = QueryStringSection;
            } else {
              req.route += c;
            }
            break;

          case QueryStringSection:
            if (c == ' ') {
              section = VersionSection;
            } else {
              req.queryString += c;
            }
            break;

          case VersionSection:
            if (c == '\n') {
              section = EmptyLineSection;
            } else {
              // Ignore everything
            }
            break;

          case HeaderNameSection:
            if (c == ' ') {
              // Ignore spaces
            } else if (c == ':') {
              section = HeaderValueSection;
            } else {
              headerName += c;
            }
            break;

          case HeaderValueSection:
            if ((c == '\r') || (c == ' ')) {
              // Ingore CR and spaces
            } else if (c == '\n') {
              if (headerName.equalsIgnoreCase("Content-Length")) {
                contentLength = headerValue.toInt();
              }
              section = EmptyLineSection;
            } else {
              headerValue += c;
            }
            break;

          case EmptyLineSection:
            if (c == '\r') {
              // Ingore CR
            } else if (c == '\n') {
              if (contentLength == 0) {
                section = FinishedSection;
              } else {
                section = BodySection;
              }
            } else {
              // New header
              headerName = "";
              headerValue = "";
              headerName += c;
              section = HeaderNameSection;
            }
            break;

          case BodySection:
            req.body += c;
            --contentLength;
            if (contentLength == 0) {
              section = FinishedSection;
            }
            break;
        }
      }
    }

    // Call user defined function when the request is complete
    if (httpServerEvent && (section == FinishedSection)) {
      httpServerEvent(req, res);
    }

    // Close connection
    client.stop();
  }
}
