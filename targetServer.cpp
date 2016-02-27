#include "imageTracker.hpp"
#include <iostream>

// Networking headers
#include <sys/socket.h>
#include <netinet/in.h>

int main(int argc, char** argv) {
  if (argc != 4) {
    std::cout << "Usage: ./targetServer teamNumber referenceImage aspectThreshold" << std::endl;
    return 1;
  }

  // Parse input
  std::string teamNumber(argv[1]);
  int splitIndex = teamNumber.size() - 2;
  std::string url = "http://10." + teamNumber.substr(0, splitIndex) + "." + teamNumber.substr(splitIndex) + ".11/mjpg/video.mjpg";
  cv::Mat Iref = cv::imread(argv[2]);
  double aspectThresh = atof(argv[3]);

  ImageTracker track(url, Iref, aspectThresh);

  int sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock < 0) {
    perror("socket() failed");
    abort();
  }


  int optval = 1;
  struct sockaddr_in sin;
  
  memset(&sin, 0, sizeof(sin));
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = INADDR_ANY;
  sin.sin_port = htons(atoi(argv[1]) + 1000);

  // Make port reusable
  if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
    perror("reuse option failed");
    abort();
  }

  if (bind(sock, (struct sockaddr *) &sin, sizeof(sin)) < 0) {
    perror("bind() failed");
    abort();
  }

  std::cout << "Listening..." << std::endl;

  char recvBuffer[1024];
  while (true) {
    struct sockaddr_in sin;
    int sin_len = sizeof(sin);

    // Blocks until packet received
    recvfrom(sock, recvBuffer, sizeof(recvBuffer), 0, (struct sockaddr *) &sin, (socklen_t*) &sin_len);
    
    // Put computed offset in a buffer
    char resultBuffer[10];
    double offset = track.getNextOffset();
    sprintf(resultBuffer, "%.6f\n", offset);

    // Send response back to client
    sendto(sock, resultBuffer, sizeof(resultBuffer), 0, (struct sockaddr*) &sin, sizeof(sin));

    // Print result to console
    std::cout << offset << std::endl;
  }

}
