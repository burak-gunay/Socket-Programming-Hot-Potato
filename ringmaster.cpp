#include <algorithm>
#include <arpa/inet.h>
#include <cstring>
#include <errno.h>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>
int errno;
int main(int argc, char *argv[]) {
  // Command line validation
  if (argc != 4) {
    std::cerr << "Please provide proper number of inputs!" << std::endl;
    exit(EXIT_FAILURE);
  }
  long int num_port = strtol(
      argv[1], NULL,
      10); // Just to check the port, not going to be used other than checking
  long int num_players = strtol(argv[2], NULL, 10);
  long int num_hops = strtol(argv[3], NULL, 10);
  if (num_port == 0) {
    printf("Provide proper port\n");
    exit(EXIT_FAILURE);
  }
  if (num_players < 1) {
    printf("Provide proper number of players\n");
    exit(EXIT_FAILURE);
  }
  if (num_hops < 0 || num_hops > 512) {
    printf("numhops should be greater/equal to 0 and less/equal to 512\n");
    exit(EXIT_FAILURE);
  }

  fd_set readfds;
  FD_ZERO(&readfds);
  struct timeval tv;
  // End validation
  srand((unsigned int)time(NULL) + num_players);
  // BEGINNING OF BOILERPLATE
  int status;
  int socket_fd;
  struct addrinfo host_info;
  struct addrinfo *host_info_list;
  const char *hostname = NULL;
  const char *port = argv[1];

  memset(&host_info, 0, sizeof(host_info)); // fills the whole struct with 0s

  host_info.ai_family = AF_UNSPEC;
  host_info.ai_socktype = SOCK_STREAM;
  host_info.ai_flags = AI_PASSIVE;

  status = getaddrinfo(hostname, port, &host_info,
                       &host_info_list); // unsure of the package above, maybe
                                         // double check if necessary?
  if (status != 0) {
    std::cerr << "Error: cannot get address info for host" << std::endl;
    std::cerr << "  (" << hostname << "," << port << ")" << std::endl;
    return -1;
  } // if

  socket_fd = socket(host_info_list->ai_family, host_info_list->ai_socktype,
                     host_info_list->ai_protocol);
  if (socket_fd == -1) {
    std::cerr << "Error: cannot create socket" << std::endl;
    std::cerr << "  (" << hostname << "," << port << ")" << std::endl;
    return -1;
  } // if

  int yes = 1;
  status = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
  status = bind(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
  if (status == -1) {
    std::cerr << "Error: cannot bind socket" << std::endl;
    std::cerr << "  (" << hostname << "," << port << ")" << std::endl;
    return -1;
  } // if

  status = listen(socket_fd, 100);
  if (status == -1) {
    std::cerr << "Error: cannot listen on socket" << std::endl;
    std::cerr << "  (" << hostname << "," << port << ")" << std::endl;
    return -1;
  } // if

  // END OF BOILERPLATE
  std::cout << "Potato Ringmaster" << std::endl
            << "Players = " << num_players << std::endl
            << "Hops = " << num_hops << std::endl;

  // Part where we start listening to the players
  // Two ints we have, num_players and num_hops
  std::vector<int> client_id_list; // add to the client list, with each player
                                   // joining us. First the setup
  std::vector<sockaddr_in> sockaddr_list;
  for (long int i = 0; i < num_players; i++) {
    struct sockaddr_storage socket_addr;
    socklen_t socket_addr_len = sizeof(socket_addr);
    int client_connection_fd;
    client_connection_fd =
        accept(socket_fd, (struct sockaddr *)&socket_addr, &socket_addr_len);
    if (client_connection_fd == -1) {
      printf("Error: cannot accept connection on socket\n");
      exit(EXIT_FAILURE);
    } // if
    struct sockaddr_in *sockaddr_ptr;
    sockaddr_ptr = (struct sockaddr_in *)&socket_addr;
    //           << inet_ntoa(sockaddr_ptr->sin_addr) << std::endl
    //           << "With this port " << sockaddr_ptr->sin_port << std::endl;

    char buffer[20];
    int test_recv = recv(client_connection_fd, buffer, 9, 0);
    if (test_recv == 0 || test_recv == -1) {
      std::cout << "Error: Could not receive the message" << std::endl;
    }
    std::string test_string(buffer, test_recv);

    // now, adjust the port number to the bind address? That should work.
    // Change std_str to char, then to uint
    char *temp = new char[test_string.length() + 1];
    std::strcpy(temp, test_string.c_str()); // now temp has char
    unsigned short port_num = atoi(temp);
    delete[] temp;
    // std::cout << "Int num is" << port_num << std::endl;
    sockaddr_ptr->sin_port = port_num; // Change to binding port
    // std::cout << "Updated port num is " << sockaddr_ptr->sin_port <<
    // std::endl;
    sockaddr_list.push_back(*sockaddr_ptr);
    client_id_list.push_back(client_connection_fd);
    FD_SET(client_connection_fd, &readfds); // set up for select
    std::cout << "Player " << client_id_list.size() - 1 << " is ready to play "
              << std::endl;
    // Now, send the player it's index
    if (send(client_connection_fd, &i, sizeof(long int), 0) <= 0) {
      std::cerr << "Could not send index properly" << std::endl;
    }
    if (send(client_connection_fd, &num_players, sizeof(long int), 0) <= 0) {
      std::cerr << "Could not send num_players properly" << std::endl;
    }
  }
  // When this is all done, set up n for select as well. Greatest + 1
  int select_n = client_id_list.back() + 1;
  tv.tv_sec = 50;
  tv.tv_usec = 500000;

  // Do another loop, send the info to connect to each of them.
  for (uint i = 0; i < sockaddr_list.size(); i++) { // The way we index them too
    if (i == 0) { // Base case, if i=0 send end of array to connect
                  // First, send port
      int sent_port = send(client_id_list[i],
                           &(sockaddr_list[sockaddr_list.size() - 1].sin_port),
                           sizeof(unsigned short), 0);
      int sent_addr = send(client_id_list[i],
                           &(sockaddr_list[sockaddr_list.size() - 1].sin_addr),
                           sizeof(struct in_addr), 0);
      if (sent_port != sizeof(unsigned short) ||
          sent_addr != sizeof(struct in_addr)) {

        std::cout << "Could not send connect details properly" << std::endl;
        std::cout << sent_port << std::endl << sent_addr << std::endl;
      }
      // int send_size =
      //     send(client_id_list[i], &(sockaddr_list[sockaddr_list.size() - 1]),
      //          sizeof(sockaddr_in), 0);
      // if (send_size != sizeof(sockaddr_in)) {
      //   std::cout << "Error: Not the whole thing could be sent" << std::endl;
      // }
    } // send last elem to i=0
    else {

      int sent_port = send(client_id_list[i], &(sockaddr_list[i - 1].sin_port),
                           sizeof(unsigned short), 0);
      int sent_addr = send(client_id_list[i], &(sockaddr_list[i - 1].sin_addr),
                           sizeof(struct in_addr), 0);
      if (sent_port != sizeof(unsigned short) ||
          sent_addr != sizeof(struct in_addr)) {
        std::cout << "Could not send connect details properly" << std::endl;
      }
      // int send_size = send(client_id_list[i], &(sockaddr_list[i - 1]),
      //                      sizeof(sockaddr_in), 0);
      // if (send_size != sizeof(sockaddr_in)) {
      //   std::cout << "Error: Not the whole thing could be sent" << std::endl;
      // }
    }
  }
  const char *connect_ch = "Con";
  const char *accept_ch = "Acc";
  // now, make them connect and accept
  for (uint i = 0; i < sockaddr_list.size(); i++) { // The way we index them too
    if (i == 0) { // Base case, if i=0 send end of array to connect
      char buffer_first[5];
      char buffer_second[5];

      if (send(client_id_list[client_id_list.size() - 1], accept_ch, 5, 0) !=
          5) {
        std::cout << "Issue with send accept" << std::endl;
      }
      if (send(client_id_list[0], connect_ch, 5, 0) != 5) {
        std::cout << "Issue with send connect" << std::endl;
      }
      int test_first = recv(client_id_list[0], buffer_first, sizeof(char), 0);
      // send and test first, with connecting one
      int test_second = recv(client_id_list[client_id_list.size() - 1],
                             buffer_second, sizeof(char), 0);
      // std::cout << "Sent sizes are " << sent_size1 << "and" << sent_size2
      //        << std::endl;
      if (buffer_first[0] != 'c') {
        std::cout << "Did not receive c" << std::endl;
      }
      if (buffer_second[0] != 'a') {
        std::cout << "Did not receive a" << std::endl;
      }
      //  std::cout << "Received from i==0" << std::endl;

      if (test_first == 0 || test_first == -1 || test_second == 0 ||
          test_second == -1) {
        perror("This happened");
        std::cout << "There is a problem with i==0" << std::endl;
        std::cout << "Test first val" << test_first << std::endl;
        std::cout << "Test second val" << test_second << std::endl;
      }
      // Recv from them back, saying ok we got it? listening back?
    } // send last elem to i=0
    else {
      // tell i to connect, i-1 to accept
      char buffer_first[5];
      char buffer_second[5];

      send(client_id_list[i - 1], accept_ch, strlen(accept_ch), 0);

      send(client_id_list[i], connect_ch, strlen(connect_ch), 0);

      int test_first = recv(client_id_list[i], buffer_first, sizeof(char), 0);
      int test_second =
          recv(client_id_list[i - 1], buffer_second, sizeof(char), 0);
      // std::cout << "Sent sizes are " << sent_size1 << "and" << sent_size2
      //          << std::endl;
      if (buffer_first[0] != 'c') {
        std::cout << "Did not receive c" << std::endl;
      }
      if (buffer_second[0] != 'a') {
        std::cout << "Did not receive a" << std::endl;
      }
      // std::cout << "Received from i!=0" << std::endl;

      if (test_first == 0 || test_first == -1 || test_second == 0 ||
          test_second == -1) {
        perror("This happened");
        std::cout << "There is a problem with i==0" << std::endl;
        std::cout << "Test first val" << test_first << std::endl;
        std::cout << "Test second val" << test_second << std::endl;
      }
    }
  }
  // First of all, randomly choose a value to sent.
  if (num_hops == 0) {
  } else {
    int initial_send = rand() % num_players;
    send(client_id_list[initial_send], &num_hops, sizeof(long int), 0);
    int rv = select(select_n, &readfds, NULL, NULL, &tv);
    int size_expected; // first receive this, then receive the rest
    std::vector<int> potato_trace;
    if (rv == -1) {
      std::cout << "error" << std::endl;
    } else if (rv == 0) {
      std::cout << "timeout" << std::endl;
    } else {
      // Now, loop, and find which socket fd sent us the packet
      for (size_t i = 0; i < client_id_list.size(); i++) {
        if (FD_ISSET(client_id_list[i], &readfds)) {
          // recv the size expected first?
          // std::cout << "Received from player " << i << std::endl;
          recv(client_id_list[i], &size_expected, sizeof(int), 0);
          potato_trace.resize(size_expected /
                              sizeof(int)); // So, it can accomodate all
                                            // the new inputs coming in
          recv(client_id_list[i], &potato_trace.data()[0], size_expected,
               MSG_WAITALL);
        }
      }
    }
    std::cout << "Trace of potato:" << std::endl;
    for (size_t i = 0; i < potato_trace.size(); i++) {
      if (i == potato_trace.size() - 1) {
        std::cout << potato_trace[i] << std::endl;
        break;
      }
      std::cout << potato_trace[i] << ",";
    }
    // Now, you can print here.
  }
  // Now, tell them all to close it down.
  long int closing_time = -1;
  for (size_t i = 0; i < client_id_list.size(); i++) {
    send(client_id_list[i], &closing_time, sizeof(long int), 0);
    // std::cout << "Told to close" << std::endl;
  }

  freeaddrinfo(host_info_list);
  close(socket_fd);
  return EXIT_SUCCESS;
}
