#ifndef __HttpServer_H__
#define __HttpServer_H__

#include <Arduino.h>

#ifdef MDUINO_PLUS
#include <Ethernet2.h>
#else
#include <Ethernet.h>
#endif

// Comment ESCAPE_FORM_STRING_VALUES definition to save some RAM and FLASH
#define ESCAPE_FORM_STRING_VALUES

// #define SET_CONNECTION_CLOSE to send http header "Connetion: close" and thus notify client that this connection will be closed by server
// or (but not both)
// #define SET_KEEP_ALIVE to send http header "Keep-Alive: timeout=1, max=1" and thus force client to close connection
#define SET_KEEP_ALIVE

class FormString : public String {
	public:
		FormString();
		explicit FormString(const String &str);

	public:
		String getValue(const String &name) const;
};

typedef struct {
	IPAddress remoteIP;
	uint16_t remotePort;
	String method;
	String route;
	FormString queryString;
	String body;
} HttpRequest;

class HttpResponse {
	public:
		explicit HttpResponse(EthernetClient &client);

	public:
		bool send(const String &body, const String &contentType, uint16_t status = 200, const String &statusText = "OK");
		bool sendStream(Stream &stream, const String &contentType);

		inline bool text(const String &body, uint16_t status = 200, const String &statusText = "OK") {
			return send(body, F("text/plain"), status, statusText);
		}

		inline bool html(const String &body, uint16_t status = 200, const String &statusText = "OK") {
			return send(body, F("text/html"), status, statusText);
		}

		bool redirect(const String &dest);

	private:
		EthernetClient &_client;
};

class HttpServer : public EthernetServer {
	private:
		enum ParseSections {
			MethodSection,
			PathSection,
			QueryStringSection,
			VersionSection,
			HeaderNameSection,
			HeaderValueSection,
			EmptyLineSection,
			BodySection,
			FinishedSection,
		};

	public:
		explicit HttpServer(uint16_t port = 80);

	public:
		void update();
};

#endif // __HttpServer_H__
