/**
    CS-11 Format
    File: mjpgserver.cpp
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
#include "mjpgserver.h"

MjpgServer::MjpgServer(int port)
{
    this->port = port;
}

MjpgServer::~MjpgServer()
{
    boost::mutex::scoped_lock l(this->global_mutex);
    std::cout << "Dismounting " << this->name << " server!";
    this->unint(); //Call the users soft unmount code
}

void MjpgServer::attach(cv::Mat (*pullframe)(void))
{
    boost::mutex::scoped_lock l(this->global_mutex);
    this->pullframe = pullframe; //Look at mainloop
}

void MjpgServer::setCapAttach(int value) //Set physical device pull with safety
{
    boost::mutex::scoped_lock l(this->global_mutex);
    this->cap.open(value);
    boost::this_thread::sleep_for(boost::chrono::milliseconds(250)); //Keeps from overreading
    this->capattach_in(); //Call default attach method
}

void MjpgServer::setCapAttach(std::string value) //Set stream to pull from
{
    boost::mutex::scoped_lock l(this->global_mutex);
    this->cap.open(value);
    boost::this_thread::sleep_for(boost::chrono::milliseconds(250));
    this->capattach_in();
}

void MjpgServer::setQuality(int quality)
{
    boost::mutex::scoped_lock l(this->global_mutex);
    this->quality = quality;
}

int MjpgServer::getQuality()
{
    boost::mutex::scoped_lock l(this->global_mutex);
    return this->quality;
}

void MjpgServer::setFPS(int fps)
{
    boost::mutex::scoped_lock l(this->global_mutex);
    this->controlfps = fps;
}

int MjpgServer::getFPS()
{
    boost::mutex::scoped_lock l(this->global_mutex);
    return this->fps;
}

void MjpgServer::setResolution(int width, int height)
{
    boost::mutex::scoped_lock l(this->global_mutex);
    this->resized[0] = width;
    this->resized[1] = height;
}

int* MjpgServer::getResolution()
{
    boost::mutex::scoped_lock l(this->global_mutex);
    return this->resized;
}

void MjpgServer::setSettle(int fps)
{
    boost::mutex::scoped_lock l(this->global_mutex);
    this->settlefps = fps;
    std::cout << "New settle fps: " << this->settlefps << std::endl;
}

void MjpgServer::setMaxConnections(int connections)
{
    boost::mutex::scoped_lock l(this->global_mutex);
    this->maxconnections = connections;
}

int MjpgServer::getMaxConnections()
{
    boost::mutex::scoped_lock l(this->global_mutex);
    return this->maxconnections;
}

int MjpgServer::getConnections()
{
    boost::mutex::scoped_lock l(this->global_mutex);
    return this->connections;
}

void MjpgServer::capattach_in()
{
    while(!cap.isOpened()) {} //Make sure we are connected before continuing
    this->setSettle((int) cap.get(cv::CAP_PROP_FPS));
    const auto proc = [this]() -> cv::Mat
    {
        cv::Mat frame;
        try {
            if(!this->cap.read(frame)) return frame;
        }
        catch (std::exception& err)
        {
            std::cerr << "Overread on stream" << std::endl;
        }
        return frame;
    };
    const auto deproc = [this]() -> void
    {
        try {
            this->cap.release();
        }
        catch(cv::Exception& err)
        {
            std::cerr << "Release capture error: " << err.what() << std::endl;
        }
    };
    static auto static_proc = proc;
    static auto static_deproc = deproc;
    cv::Mat (*ptr) () = []() -> cv::Mat { return static_proc(); }; //Create the captured lambdas into function pointers
    void (*dptr) () = []() -> void { return static_deproc(); };
    this->attach(ptr);
    this->detacher(dptr);
}

void MjpgServer::detacher(void (*detacher)(void))
{
    boost::mutex::scoped_lock l(this->global_mutex);
    this->unint = detacher;
}

void MjpgServer::setName(std::string new_name)
{
    boost::mutex::scoped_lock l(this->global_mutex);
    this->name = new_name;
}

std::string MjpgServer::convertString()
{
    std::vector<uchar> buff;
    if(this->resized[0] > 0)
    {
        cv::resize(this->curframe, this->curframe, cv::Size(this->resized[0], this->resized[1]), 0, 0, cv::INTER_LINEAR); // If resize then do so
    }

    if(this->quality > -1) //If specified quality then do so
    {
        std::vector<int> compression_params;
        compression_params.push_back(cv::IMWRITE_JPEG_QUALITY);
        compression_params.push_back(this->quality);
        cv::imencode(".jpg", this->curframe, buff, compression_params);
    }
    else
    {
        cv::imencode(".jpg", this->curframe, buff);
    }
    std::string content(buff.begin(), buff.end());
    return content;
}

void MjpgServer::handleJpg(asio::ip::tcp::socket &socket)
{
    boost::mutex mutex;
    std::cout << "Client requested single image!" << std::endl;
    boost::mutex::scoped_lock l(mutex);
    this->connections += 1;
    try
    {
        this->curframe = this->pullframe();
        std::string content = this->convertString();
        std::stringstream response;
        response << "HTTP/1.1 200 OK\r\nContent-Type: image/jpeg\r\nServer: " << this->host_name;
        response << "\r\nContent-Length: " << content.length() << "\r\n\r\n" << content;
        if(!sendresponse(socket, response.str())) throw std::invalid_argument("send error");
    }
    catch(std::exception& err)
    {
        std::cerr << "Error sending image to client!" << std::endl;
        std::string resp = "<p>Failed sending image</p>";
        sendError(socket, resp);
    }
    this->connections -= 1;
}

void MjpgServer::mainPullLoop()
{
    boost::mutex mutex;

    mutex.lock();
    this->pullcap = true;
    mutex.unlock();

    for(int test = 0; test < 3; test++)
    {
        mutex.lock();
        try
        {
            this->pullframe();
        }
        catch(std::exception& err) {}
        mutex.unlock();
        boost::this_thread::sleep_for(boost::chrono::milliseconds(50));
    }

    std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
    std::chrono::high_resolution_clock::time_point now = start;
    while(1)
    {
        now = std::chrono::high_resolution_clock::now();
        float delta = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
        start = now;
        float timepoint = 1000 / (float) (this->settlefps > 0) ?
                          this->settlefps : (this->controlfps > 0) ?
                          this->controlfps : 1000;
        int sleepoint = 2;
        if(delta < timepoint)
        {
            sleepoint = (((int) (timepoint - delta)) * 2) - 3;
        }
        boost::this_thread::sleep_for(boost::chrono::milliseconds(sleepoint));
        mutex.lock();
        try
        {
            this->curframe = this->pullframe();
            if(!this->curframe.empty())
                this->content = this->convertString();
        }
        catch(std::exception& pullerror) {
            std::cerr << "Image pull error: " << pullerror.what() << std::endl;
        }
        mutex.unlock();
    }
    mutex.lock();
    this->pullcap = false;
    mutex.unlock();
}

void MjpgServer::handleMjpg(asio::ip::tcp::socket &socket)
{
    //Tell client mjpg stream is going to be sent
    std::stringstream respcompile;
    respcompile << "HTTP/1.1 200 OK\r\nContent-Type: multipart/x-mixed-replace; boundary=";
    respcompile << this->boundary << "\r\nServer: " << this->host_name;
    respcompile << "\r\n\r\n";
    std::string initresponse = respcompile.str();
    boost::mutex mutex;
    this->connections += 1;
    if(!sendresponse(socket, initresponse))
    {
        std::cerr << "Failed to initiate stream with client... removing client" << std::endl;
        return;
    }
    else
        std::cout << "Client connected to stream!" << std::endl;
    long failcount = 0;
    static int frames = 0;
    std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();

    if(!this->pullcap)
    {
        boost::thread(boost::bind(&MjpgServer::mainPullLoop, this));
    }

    std::chrono::high_resolution_clock::time_point point = std::chrono::high_resolution_clock::now();
    std::chrono::high_resolution_clock::time_point now = point;
    while(1) // Loop forever
    {
        try
        {
            int sleepint = 2; //Default sleep when target not specified
            {
                //scoped lock without delay
                boost::mutex::scoped_lock l(mutex);
                if(this->content.length() < 3)  //this->curframe.empty()) {
                {
                    boost::this_thread::sleep_for(boost::chrono::milliseconds(2));
                    continue;
                }
                std::stringstream response;
                response << this->boundary << "\r\nContent-Type: image/jpeg\r\nContent-Length: " << this->content.length() << "\r\n\r\n" << this->content;
                if(!sendresponse(socket, response.str()))
                {
                    if(failcount++ > this->maxfailpackets) break;
                }
                now = std::chrono::high_resolution_clock::now();
                float delta = std::chrono::duration_cast<std::chrono::milliseconds>(now - point).count();
                point = now;
                float timepoint = 1000.0f / (float) this->controlfps;
                if(delta < timepoint && this->controlfps > 0)
                {
                    sleepint = (((int) (timepoint - delta)) * 2);
                }
                auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
                frames++;
                if(duration > 250 && frames > this->samplefps)
                {
                    this->fps = (float) ((frames * 1000) / duration);
                    start = now;
                    frames = 0;
                }
            }
            boost::this_thread::sleep_for(boost::chrono::milliseconds(sleepint));
        }
        catch(cv::Exception& err)
        {
            std::cout << "CV ERROR: " << err.what() << std::endl;
        }
        catch(std::exception& err)
        {
            std::cout << "Client disconnect" << std::endl;
            break;
        }
    }
    this->connections -= 1;
}

void MjpgServer::handleHtml(asio::ip::tcp::socket &socket, std::string& root) //Look at onAccept
{
    boost::mutex mutex;
    boost::mutex::scoped_lock l(mutex);
    std::stringstream p_con;
    p_con << "<html><head></head><body><img src=\"" << root << "\"/></body></html>";
    std::string main_content = p_con.str();
    std::stringstream content;
    content << "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: " << main_content.length();
    content << "\r\nServer: " << this->host_name;
    content << "\r\n\r\n" << main_content;
    sendresponse(socket, content.str());
}

void MjpgServer::sendSimple(asio::ip::tcp::socket &socket, std::string& simple) //Look at onAccept
{
    std::stringstream content;
    content << "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: " << simple.length();
    content << "\r\nServer: " << this->host_name;
    content << "\r\n\r\n" << simple;
    sendresponse(socket, content.str());
}

void MjpgServer::onAccept(asio::ip::tcp::socket &socket) //Look at onAccept
{
    boost::mutex mutex;

    while(1)
    {
        std::string httprequest;
        std::vector<std::string> reqs;
        std::map<std::string, std::string> headers;
        try
        {
            httprequest = this->readrequest(socket);
            if(httprequest == this->badread) throw std::invalid_argument("bad read");
            headers = this->parseheaders(httprequest);
            reqs = typeReq(httprequest);
        }
        catch(std::exception& err)
        {
            std::cerr << "String parse error" << std::endl;
            break;
        }

        try
        {
            if(reqs[2] != "HTTP/1.1") throw std::invalid_argument("HTTP");
        }
        catch(std::exception& err)
        {
            std::cerr << "Bad HTTP request" << std::endl;
            std::string resp = "<p>Request needs to be <b>HTTP/1.1</b></p>";
            sendError(socket, resp);
            break;
        }

        std::string req_type;
        std::string path;
        std::string extension;

        try
        {
            req_type = reqs[0];
            std::stringstream pathgen;
            pathgen << "http://" << headers["Host"];
            pathgen << reqs[1];
            path = pathgen.str();
            extension = reqs[1];
        }
        catch(std::exception& err)
        {
            std::cerr << "String parse error" << std::endl;
            std::string resp = "<p>Request faced internal server error <b>(Couldn't part headers)</b></p>";
            sendError(socket, resp);
            continue;
        }

        try
        {
            if(extension == "/mjpg")
            {
                if(this->maxconnections > 0 && this->connections >= this->maxconnections)
                {
                    this->sendError(socket, this->tooManyErr);
                    break;
                }
                try
                {
                    this->handleMjpg(socket);
                }
                catch(std::exception& mjpgerr)
                {
                    std::cerr << "Error handling mjpg stream with client: " << mjpgerr.what() << std::endl;
                }
                break;
            }
            else if(extension == "/" || extension == "/html")
            {
                if(this->maxconnections > 0 && this->connections >= this->maxconnections)
                {
                    this->sendError(socket, this->tooManyErr);
                    break;
                }
                try
                {
                    std::stringstream mjpgpath;
                    mjpgpath << "http://" << headers["Host"] << "/mjpg";
                    std::string newpath(mjpgpath.str());
                    this->handleHtml(socket, newpath);
                }
                catch(std::exception& errsend)
                {
                    std::cerr << "Error html request: " << errsend.what() << std::endl;
                    break;
                }
            }
            else if(extension == "/jpg") //Send single image
            {
                try
                {
                    this->handleJpg(socket);
                }
                catch(std::exception& imageerr)
                {
                    std::cerr << "Error jpg send: " << imageerr.what() << std::endl;
                }
                break;
            }
            else if(extension == "/fps") //REST control fps
            {
                std::string tosend;
                boost::mutex::scoped_lock l(mutex);
                try
                {
                    if(req_type == "GET")
                    {
                        std::cout << "Requested to get fps" << std::endl;
                        std::stringstream ss;
                        ss << (int) this->fps; //Turn the float to int to string
                        tosend = ss.str();
                        this->sendSimple(socket, tosend);
                    }
                    else if(req_type == "POST")
                    {
                        std::string body = this->getBody(httprequest);
                        this->controlfps = atoi(body.c_str());
                        tosend = ""; //Send empty response since it's a simple response
                        this->sendSimple(socket, tosend);
                        std::cout << "Requested to set fps to: " << this->controlfps << " Completed" << std::endl;
                    }
                    else
                    {
                        this->sendError(socket, this->defErr);
                    }
                    break;
                }
                catch(std::exception& err)
                {
                    std::cerr << "Bad request on resolution" << std::endl;
                    this->sendError(socket, this->defErr);
                }
            }
            else if(extension == "/quality")
            {
                std::string tosend;
                boost::mutex::scoped_lock l(mutex);
                try
                {
                    if(req_type == "GET")
                    {
                        std::cout << "Requested to get quality" << std::endl;
                        std::stringstream ss;
                        ss << (int) this->quality;
                        tosend = ss.str();
                        this->sendSimple(socket, tosend);
                    }
                    else if(req_type == "POST")
                    {
                        std::string body = this->getBody(httprequest);
                        this->quality = atoi(body.c_str());
                        tosend = "";
                        this->sendSimple(socket, tosend);
                        std::cout << "Requested to set quality to: " << this->quality << " Completed" << std::endl;
                    }
                    else
                    {
                        this->sendError(socket, this->defErr);
                    }
                    break;
                }
                catch(std::exception& err)
                {
                    std::cerr << "Bad request on quality" << std::endl;
                    this->sendError(socket, this->defErr);
                }
            }
            else if(extension == "/connections")
            {
                std::string tosend;
                boost::mutex::scoped_lock l(mutex);
                try
                {
                    if(req_type == "GET")
                    {
                        std::cout << "Requested to get connections" << std::endl;
                        std::stringstream ss;
                        ss << this->connections;
                        tosend = ss.str();
                        this->sendSimple(socket, tosend);
                    }
                    else if(req_type == "POST")
                    {
                        std::string body = this->getBody(httprequest);
                        this->maxconnections = atoi(body.c_str());
                        tosend = "";
                        this->sendSimple(socket, tosend);
                        std::cout << "Requested to set max connections to: " << this->maxconnections << " Completed" << std::endl;
                    }
                    else
                    {
                        this->sendError(socket, this->defErr);
                    }
                    break;
                }
                catch(std::exception& err)
                {
                    std::cerr << "Bad request on connections" << std::endl;
                }
            }
            else if(extension == "/resolution")
            {
                std::string tosend;
                boost::mutex::scoped_lock l(mutex);
                try
                {
                    if(req_type == "GET")
                    {
                        std::cout << "Requested to get resolution" << std::endl;
                        std::stringstream ss;
                        int width = this->curframe.cols;
                        int height = this->curframe.rows;
                        ss << width << "x" << height;
                        tosend = ss.str();
                        this->sendSimple(socket, tosend);
                    }
                    else if(req_type == "POST")
                    {
                        std::string body = this->getBody(httprequest);
                        std::string dim = body.substr(0, body.find("x"));
                        this->resized[0] = atoi(dim.c_str());
                        dim = body.substr(body.find("x") + 1);
                        this->resized[1] = atoi(dim.c_str());
                        tosend = "";
                        this->sendSimple(socket, tosend);
                        std::cout << "Requested to set resolution to: " << body << " Completed" << std::endl;
                    }
                    else
                    {
                        this->sendError(socket, this->defErr);
                    }
                    break;
                }
                catch(std::exception& err)
                {
                    std::cerr << "Bad request on resolution" << std::endl;
                    this->sendError(socket, this->defErr);
                }
            }
            else
            {
                std::string resp = "<p>404 Page not found! please use <b>.../mjpg, .../html, .../jpg or controls (fps, quality, resolution)</b></p>";
                sendError(socket, resp);
                break;
            }
        }
        catch(std::exception& err)
        {
            std::cerr << "Loop error" << std::endl;
            continue;
        }
    }
    try
    {
        socket.close();
    }
    catch(std::exception& safetyclose) {}
}

std::map<std::string, std::string> MjpgServer::parseheaders(const std::string response)
{
    std::map<std::string, std::string> mapper;
    std::istringstream resp(response);
    std::string header;
    std::string::size_type index;
    while (std::getline(resp, header) && header != "\r")
    {
        index = header.find(':', 0);
        if(index != std::string::npos)
        {
            mapper.insert(std::make_pair(
                              boost::algorithm::trim_copy(header.substr(0, index)),
                              boost::algorithm::trim_copy(header.substr(index + 1))
                          ));
        }
    }
    return mapper;
}

std::string MjpgServer::getBody(std::string &request)
{
    return request.substr(request.find("\r\n\r\n") + 4, request.rfind("\r\n"));
}

std::vector<std::string> MjpgServer::typeReq(std::string request)
{
    std::vector<std::string> ret;

    try
    {
        std::string temp;
        std::stringstream strings(request);
        while(strings >> temp) ret.push_back(temp);
        return ret;
    }
    catch(std::exception& err)
    {
        std::vector<std::string> ret2;
        ret2.push_back(this->badread);
        return ret2;
    }
}

std::string MjpgServer::readrequest(asio::ip::tcp::socket &socket)
{
    try
    {
        asio::streambuf buf;
        asio::read_until( socket, buf, "\r\n" );
        std::string data = asio::buffer_cast<const char*>(buf.data());
        return data;
    }
    catch(std::exception& err)
    {
        std::cerr << "Request cancelled" << std::endl;
        return this->badread;
    }
}

bool MjpgServer::sendresponse(asio::ip::tcp::socket &socket, const std::string& str)
{
    try
    {
        const std::string msg = str + "\r\n";
        asio::write( socket, asio::buffer(msg));
        return true;
    }
    catch(std::exception& err)
    {
        std::cerr << "Client write fail, pipe broken" << std::endl;
        return false;
    }
}

void MjpgServer::sendError(asio::ip::tcp::socket & socket, std::string &message)
{
    std::string content = "<html><body><h1>" + this->name + " error:</h1>";
    std::stringstream bad;
    bad << "HTTP/1.1 500\r\nContent-Type: text/html\r\nContent-Length: " << (content.length() + message.length()) << "\r\n\r\n" << content << message << "</body></html>\r\n";
    sendresponse(socket, content);
}

void MjpgServer::run(bool threaded_start) {
    if(threaded_start) {
        boost::thread t(boost::bind(&MjpgServer::run, this));
        boost::thread(boost::bind(&MjpgServer::mainPullLoop, this));
    } else {
        this->run();
    }
}


void MjpgServer::run()
{
    std::cout << "Welcome to: " << this->name << std::endl << "Waiting for a clients..." << std::endl;
    asio::io_service io_service;
    MjpgServer::server s(io_service, this, this->port);
    io_service.run();
}

void MjpgServer::session::start(MjpgServer *server)
{
    try
    {
        this->master = server;
        boost::thread t(boost::bind(&MjpgServer::onAccept, server, boost::ref(this->socket_)));
    }
    catch(...)
    {
        std::cout << "Caught threading problem not quiting though..." << std::endl;
        delete this;
    }
}

tcp::socket& MjpgServer::session::socket()
{
    return this->socket_;
}

void MjpgServer::server::start_accept()
{
    session* new_session = new session(io_service_);
    acceptor_.async_accept(new_session->socket(),
                           boost::bind(&server::handle_accept, this, new_session,
                                       boost::asio::placeholders::error));
}

void MjpgServer::server::cleanup()
{
    this->io_service_.stop();
}

void MjpgServer::server::handle_accept(session* new_session, const boost::system::error_code& error)
{
    if (!error)
    {
        new_session->start(this->master);
    }
    else
    {
        delete new_session;
    }

    start_accept();
}
