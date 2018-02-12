//STANDARD INCLUDES
#include <iostream>
#include <iomanip>
#include <sstream>
#include <ctime>
#include <functional>

//BOOST INCLUDES
#include <boost/lexical_cast.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/chrono.hpp>

//CPPREST INCLUDES
#include <cpprest/http_listener.h>
#include <cpprest/http_client.h>
#include <cpprest/http_headers.h>
#include <cpprest/json.h>
#pragma comment(lib, "cpprest110_1_1")

//MOVE TO HEADER
#define MJPEGSERVER_BINDADDR "http://*/"
#define MJPEGSERVER_DEBUG true 

using namespace web::http::experimental::listener;
using namespace web::http;
using namespace web;

//MOVE TO HEADER
namespace mjpeghandler {
	template<typename T>
	void debug(T);

	class MjpegHandler {
	public:
		MjpegHandler(const std::string &bindAddr = MJPEGSERVER_BINDADDR);
		~MjpegHandler();

		void handle_main(http_request);
		
		void startHandler();
	private:
		http_listener *__listener;
		concurrency::streams::ostream stream;
	};
}


namespace mjpeghandler {

	MjpegHandler::MjpegHandler(const std::string &bindAddr) {
		std::cout << bindAddr << std::endl;
		this->__listener = new http_listener(bindAddr);
		this->__listener->support(methods::GET, std::bind(
				&mjpeghandler::MjpegHandler::handle_main, this, std::placeholders::_1));
	}

	MjpegHandler::~MjpegHandler() {
		delete this->__listener;
	}

	void MjpegHandler::handle_main(http_request request) {
		std::cout << "GOT GET REQUEST" << std::endl;
		request.extract_string().then([](std::string body) {
			std::cout << "BODY: " << body << std::endl;
		});

		/*request.get_response().then([](http_response response) {
			concurrency::streams::ostream stream;
			
			stream.print("YAAAAAAY");

			response.set_body(ostream);
			response.status_code();


		});*/

		//http_response response;
		//response.set_body("GOT IT");
		//response.set_status_code(status_codes::OK);


		//stream.print_line("YAAY");

		//response.set_body("YAAY");
		//request.reply(response);
		request.set_response_stream(stream);
		//request.get_response();
		//request.reply(stream);
		
		/*request.get_response().then([](http_response response) {
			concurrency::streams::istream stream;

			response.set_body(stream);
			response.status_code();
		});*/

		request._reply_if_not_already(2);
		
	}

	void MjpegHandler::startHandler() {
		this->__listener->open().wait();
		std::cout << "Starting to wait" << std::endl;
		
		while(1) {
			if(stream.is_open()) {
				stream.print_line("TEST");
			} else {
				std::cout << "Stream is not open" << std::endl;
			}
			boost::this_thread::sleep_for(boost::chrono::milliseconds(1000));
		}
	}
	
	template<typename T>
	void debug(T toDebug) {
		std::cout << "MJPEGSERVER: " << toDebug << std::endl;
	}
}

int main() {
	mjpeghandler::MjpegHandler handler("http://localhost:8081/test");
	handler.startHandler();
	while(1);
}
