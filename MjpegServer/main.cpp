#include "mjpgserver.h" //MjpgServer header

int main() {
    MjpgServer server(8081); // Start server on pot 8081
    server.setQuality(1); // Set jpeg quality to 1 (0 - 100)
    server.setResolution(1280, 720); // Set stream resolution to 1280x720
    server.setFPS(15); // Set target fps to 15
    server.setCapAttach(0); // Attach webcam id 0 to stream
    server.run(); //Run stream forever (until fatal)
    return 0;
}
