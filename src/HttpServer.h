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

class FormString : public String {
	public:
		FormString();
		explicit FormString(const String &str);

	public:
		String getValue(const String &name);
		String getValue(const __FlashStringHelper *name);
};

typedef struct {
	String method;
	String route;
	FormString queryString;
	String body;
} HttpRequest;

class HttpResponse {
	public:
		explicit HttpResponse(EthernetClient &client);

	public:
		bool send(const String &body, const String &contentType, uint16_t status, const String &statusText);
		bool send(const __FlashStringHelper * body, const __FlashStringHelper *contentType, uint16_t status, const __FlashStringHelper *statusText);
		bool sendStream(const Stream &stream, const String &contentType);

		inline bool send(const String &body, const String &contentType) {
			return send(body, contentType, 200, "OK");
		}
		inline bool send(const __FlashStringHelper * body, const __FlashStringHelper *contentType) {
			return send(body, contentType, 200, F("OK"));
		}

		inline bool text(const String &body) {
			return send(body, "text/plain");
		}
		inline bool text(const __FlashStringHelper *body) {
			return send(body, F("text/plain"));
		}
		inline bool text(const String &body, uint16_t status, const String &statusText) {
			return send(body, "text/plain", status, statusText);
		}
		inline bool text(const __FlashStringHelper *body, uint16_t status, const __FlashStringHelper *statusText) {
			return send(body, F("text/plain"), status, statusText);
		}

		inline bool html(const String &body) {
			return send(body, "text/html");
		}
		inline bool html(const __FlashStringHelper *body) {
			return send(body, F("text/html"));
		}
		inline bool html(const String &body, uint16_t status, const String &statusText) {
			return send(body, "text/html", status, statusText);
		}
		inline bool html(const __FlashStringHelper *body, uint16_t status, const __FlashStringHelper *statusText) {
			return send(body, F("text/html"), status, statusText);
		}

		bool redirect(const String &dest);
		bool redirect(const __FlashStringHelper *dest);

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
