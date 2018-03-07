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

////////////////////////////////////////////////////////////////////////////////////////////////////
HttpResponse::HttpResponse(EthernetClient &client) : _client(client) {

}

////////////////////////////////////////////////////////////////////////////////////////////////////
bool HttpResponse::send(const String &body, const String &contentType, uint16_t status, const String &statusText) {
	if (!_client.connected()) {
		return false;
	}

	// Headers
	_client.print("HTTP/1.1 ");
	_client.print(status);
	_client.print(' ');
	_client.println(statusText);
	// Content type
	_client.print("Content-Type: ");
	_client.println(contentType);
	// Content length
	_client.print("Content-Length: ");
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
bool HttpResponse::redirect(const String &dest) {
	if (!_client.connected()) {
		return false;
	}

	// Headers
	_client.println("HTTP/1.1 303 See Other");
	// Content type
	_client.print("Location: ");
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
