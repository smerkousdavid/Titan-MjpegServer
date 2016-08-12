/**
    CS-11 Format
    File: mjpgserver.h
    Purpose: Capture/process mats and display on http stream

    @author David Smerkous
    @version 1.0 8/11/2016

    License: MIT License (MIT)
    Copyright (c) 2016 David Smerkous

    Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
    INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
    WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#ifndef MJPGSERVER_H_
#define MJPGSERVER_H_

#pragma once

#include <iostream>
#include <cstdlib>
#include <string>
#include <cstddef>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/video.hpp>
#include <chrono>
#include <functional>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <future>
#include <boost/chrono.hpp>
#include <unistd.h>


namespace asio = boost::asio;
using boost::asio::ip::tcp;

class MjpgServer
{
    int port = 80;
    std::string name = "Titan 5431 MjpgServer";
    std::string boundary = "--titanboundary";
    std::string badread = "&1!ERR!1&";
    std::string host_name = "Titan 5431 HttpServer 1.0 (Usage for MJPEG)";
    std::string defErr = "<p>Bad request</p>";
    std::string tooManyErr = "<p>There are <b>too many</b> connections on the line!</p>";
    long maxfailpackets = 30;
    float fps = 0.0f;
    int controlfps = -1;
    int samplefps = 50;
    int settlefps = -1;
    int quality = -1;
    int connections = 0;
    int maxconnections = -1;
    int resized[2] = {-1, -1};
    cv::VideoCapture cap;
    bool pullcap = false;
    std::string content;
    boost::mutex global_mutex;

public:
    //! MjpgServer constructor
    /*!
    This will start the mjpgserver on the specified port

    @param port an integer value for the port to
    @return the MjpgServer object
    */
    MjpgServer(int);

    //! MjpgServer custom deconstructor
    /*!
    This will call a custom user release and if non provided
    Attempt to release any OpenCv cap provided
    */
    ~MjpgServer(void);

    //! Sets the printed name (html/console)
    /*!
    When the program is ran it will print to html the name and console.
    Not too important

    @param new_name string to set the new name
    */
    void setName(std::string);

    //! Attaches a pull method to the server
    /*!
    Connects an OpenCv primitive method to a function pointer
    That will called by the main loops ( @see setSettle() ) to
    adjust threaded pull rate. Keep the map in BGR color space if
    you want the results to come out as expected. Please place inits
    Out of this function. The pointer function/lambda must be in a
    static contex

    @param pullframe a function pointer to a mat return

    Example: { @code  const auto proc = [&cap]() -> cv::Mat
    {
        cv::Mat frame;
        if(!cap.read(frame)) std::cout << "bad frame" << std::endl;
        return frame;
    };
    static auto static_proc = proc;
    cv::Mat (*ptr) () = []() -> cv::Mat { return static_proc(); };
    server.attach(ptr);
    }
    */
    void attach(cv::Mat (*pullframe)(void));

    //! Custom release of stream reader
    /*!
    A custom detacher that will release the stream
    So you aren't too hard on V4L if you are using it.
    It's nice to be soft sometimes

    @param detacher function pointer to custom detacer (Must be static)
    @see attach()
    */
    void detacher(void (*detacher)(void));

    //! Run the server
    /*!
    Runs the server pool and start calling your attach
    method as soon as the first client connects. I recommend that
    you run your pull/capture/processing method in a seperate thread
    so your application doesn't require a client connection, Or you
    can default the bool param to do it for you
    @param threaded_start
    */
    void run(bool);

    //! Run the server (not threaded)
    /*!
    Runs the server pool with your attach method
    @see run(bool)
    */
    void run(void);

    //! Attach default device reader
    /*!
    Same as opencv video cap but with better reading capabilites
    and crash handeling. Try { @code server.setCapAttach(0); } to test
    your server with your default webcam.

    @param value attaches to camera device num
    */
    void setCapAttach(int);

    //! Attach to webstream or file
    /*!
    Sets the attach method to an opencv stream capture. This could
    be extremely useful if you want to redirect a camera stream to
    your localhost, So whatever server you're attached to doesn't

    @param value attaches server to existing stream

    eat up your bandwidth and you just rebroadcast it locally. ( @see setCapAttach(int) )
    Example:
    { @code server.setCapAttach("http://localhost:8080/mjpg"); }

    */
    void setCapAttach(std::string);

    //! Set stream quality (0 - 100)
    /*!
    Sets the server stream jpeg quality/compression value
    This can be set realtime but is preffered to be set before hand.
    To stop regulating quality set the value to -1

    @param quality an integer between 0 - 100 or -1 (low - high) - unregulated
    */
    void setQuality(int);

    //! Get current quality of stream
    /*!
    Generally not used but if the REST client has been updated
    and you want to be strict about the updates you can have a pull
    method to reset the stream back to desired values

    @return an integer of the current value between 0 - 100 (low - high) or -1 which is unregulated
    */
    int getQuality(void);

    //! Set max frame rate (Delta controlled)
    /*!
    Sets the servers optimal frame pull rate it's recommended
    That this value stays at a lower value because depending on the
    cpu this can eat up to 90% of your cpu since the server will try
    it's best to get to that value. This can also be regulated via the
    REST client;

    @param fps an integer between 1 > ... or -1 to set unregulated which is 2ms delay
    */
    void setFPS(int);

    //! Get calculated fps
    /*!
    Get current real calculated fps value, trust me on this when I
    say this works the best on one client. Because you will then get
    the most accurate result.

    @return an integer of the current fps
    */
    int getFPS(void);

    //! Sets the output stream width and height
    /*!
    Try not to use this setting by default because resizing will introduce a new type
    of lag and cpu usage that might vary depending on the quality and current size of
    the stream. Try adding it "naturally" to the attached pull method. Set both of them
    to -1 to unregulate which will remove the slight lag

    @param width integer of the width in pixels
    @param height integer of the height in pixels
    */
    void setResolution(int, int);

    //!Get the set resolution
    /*!
    Get the current set output stream resolution

    @return an integer array of size 2 like such { width, height }
    */
    int* getResolution(void);

    //!Clock fps "time" of camera
    /*!
    First fact is that if you've used a setCapAttach do not touch this method
    because it will be completely redundant and unused. Either than that since
    It's extremely annoying to sync the internal pull thread frames between clients.
    I would use this to keep an anonymous low frame pull rate according to camera specs.
    So that you don't over eat your cpu. Set this to actuall hardware spec or pre buffer
    read rate. So you can get the most updated frames...

    @param fps an integer of real pull fps
    */
    void setSettle(int);

    //! Set max connections allowed on stream
    /*!
    This will limit the amount of JPEG or MJPEG streams allowed to be connected to the
    server. This number will reset as soon as a client disconnects

    @param connections max client connections number between 1 > ... or -1 for unlimited
    */
    void setMaxConnections(int);

    //! Get max number of connections
    /*!
    Get the max number of clients that can connect to the stream. It will be -1 if the
    max is unregulated. It should never be 0 then what's the point of running this server...

    @return the max amount
    */
    int getMaxConnections(void);

    //! Get current connections amount
    /*!
    This will return the amount of clients that are currently connected

    @return integer of the amount of clients connected
    */
    int getConnections();

private:
    //!Global thread shared frame
    cv::Mat curframe;

    //!Internal attach method for getting OpenCv Mat
    cv::Mat (*pullframe)(void);

    //!Internal detach method for releasing cameras
    void (*unint)(void);

    //!A string map of the headers by key and value
    std::map<std::string, std::string> parseheaders(const std::string);

    //!Pulls the entire request till socket close
    std::string readrequest(asio::ip::tcp::socket &);

    //!Sends the http response with closure eof
    bool sendresponse(asio::ip::tcp::socket &, const std::string&);

    //!Sends a default error with an html based message
    void sendError(asio::ip::tcp::socket &, std::string &);

    //!When the extension is /html run the default html handler (Doesn't break connection)
    void handleHtml(asio::ip::tcp::socket &, std::string&);

    //!When the extension is /mjpg run the mjpg server stream (Closes on end of request)
    void handleMjpg(asio::ip::tcp::socket &);

    //!When the extension is /jpg run the single image response (Closes on end of request)
    void handleJpg(asio::ip::tcp::socket &);

    //!Sends a simple REST text/plain response to the client
    void sendSimple(asio::ip::tcp::socket &, std::string&);

    //!Turns the global OpenCv Mat into a byte encoded string
    std::string convertString(void);

    //!Splits the response by spaces to retrieve header data
    std::vector<std::string> typeReq(std::string);

    //!Removes the requests headers and newline characters to just retrieve the body
    std::string getBody(std::string &);

    //!Internal method to attach cap to the pull method
    void capattach_in(void);

    //!On session successful completion of socket run the mjpgserver main code
    void onAccept(asio::ip::tcp::socket &);

    //!Main loop to pull the user defined methods in seperate buffer free thread
    void mainPullLoop(void);

    //!Async session provider
    /*!
    On pooled thread accepted socket create a new session thread with client socket
    When completed cleanup the async socket (Used in a more synchronous method)
    */
    class session
    {
    public:
        //!Init session with io service
        session(boost::asio::io_service& io_service)
            : socket_(io_service) {}
        //!Start new mjpg thread
        void start(MjpgServer *);
        //!The core of mjpgserver the connection
        tcp::socket &socket();
    private:
        tcp::socket socket_;
        MjpgServer *master;
    };

    //!Init thread of mjpgserver
    /*!
    This will start the server acceptor and session cleanup loop
    */
    class server
    {
    public:
        server(boost::asio::io_service& io_service, MjpgServer *server, short port) :
            io_service_(io_service), acceptor_(io_service, tcp::endpoint(tcp::v4(), port))
        {
            //Set the init method to the
            this->master = server;
            start_accept();
        }

        void cleanup(void);
    private:
        //This is self explanatory
        void start_accept();
        void handle_accept(session*, const boost::system::error_code&);
        boost::asio::io_service& io_service_;
        tcp::acceptor acceptor_;
        //!Class pointer to main mjpgserver code
        MjpgServer *master;
    };
};

#endif  // MJPGSERVER_H_
