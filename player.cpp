#include <algorithm>
#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <netdb.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

int main(int argc, char *argv[]) {
  fd_set readfds;
  struct timeval tv;
  FD_ZERO(&readfds);
  // Command Line Validation
  if (argc != 3) {
    std::cerr << "Please provide proper number of inputs!" << std::endl;
    exit(EXIT_FAILURE);
  } // End of Command Line Validation
  // Listening Part of It
  // First, set up the socket that the player will listen on
  int status_listen;
  int socket_fd_listen;
  struct addrinfo host_info_listen;
  struct addrinfo *host_info_list_listen;
  // Can try to change this, to not null but gethostname. Might make diff?
  const char *hostname_listen = "0.0.0.0";
  // const char *port_listen = "0";

  memset(&host_info_listen, 0,
         sizeof(host_info_listen)); // fills the whole struct with 0s
  host_info_listen.ai_family = AF_UNSPEC;
  host_info_listen.ai_socktype = SOCK_STREAM;
  host_info_listen.ai_flags = AI_PASSIVE;
  status_listen = -1;
  int start_number = 6000;
  std::string start_string = std::to_string(start_number);
  while (status_listen == -1) {
    char new_port_listen[20];
    strcpy(new_port_listen, start_string.c_str());
    status_listen =
        getaddrinfo(hostname_listen, new_port_listen, &host_info_listen,
                    &host_info_list_listen); // unsure of the package above,
                                             // maybe double check if necessary?
    if (status_listen != 0) {
      std::cerr << "Error Listen: cannot get address info for host"
                << std::endl;

      return -1;
    } // if
    socket_fd_listen = socket(host_info_list_listen->ai_family,
                              host_info_list_listen->ai_socktype,
                              host_info_list_listen->ai_protocol);
    if (socket_fd_listen == -1) {
      std::cerr << "Error listen: cannot create socket" << std::endl;

      return -1;
    } // if
    // Left at bind
    int new_yes = 1;
    status_listen = setsockopt(socket_fd_listen, SOL_SOCKET, SO_REUSEADDR,
                               &new_yes, sizeof(int));
    status_listen = bind(socket_fd_listen, host_info_list_listen->ai_addr,
                         host_info_list_listen->ai_addrlen);
    if (status_listen == -1) {
      start_number++;
      start_string = std::to_string(start_number); // incremented it now
    }
    if (status_listen != -1) {
      // std::cout << "This is available port: " << start_number << std::endl;
      break;
    }
  }

  status_listen = listen(socket_fd_listen, 2);
  if (status_listen == -1) {
    std::cerr << "Error Listen: cannot listen on socket" << std::endl;

    return -1;
  } // if

  // std::cout << "Listening for p2p comm" << std::endl;
  struct sockaddr_storage socket_addr_listen;
  socklen_t socket_addr_len_listen = sizeof(socket_addr_listen);
  getsockname(socket_fd_listen, (struct sockaddr *)&socket_addr_listen,
              &socket_addr_len_listen);
  struct sockaddr_in *sockaddr_ptr_listen;
  sockaddr_ptr_listen = (struct sockaddr_in *)&socket_addr_listen;
  sockaddr_ptr_listen->sin_port =
      start_number; // Update this to the number we have found!
  // std::cout << "On this address " << sockaddr_ptr_listen->sin_port <<
  // std::endl;
  unsigned short port_bind_num = sockaddr_ptr_listen->sin_port;
  // This is the address that we will be listening

  // BOILERPLATE - Server connection
  int status;
  int server_socket_fd;
  struct addrinfo host_info;
  struct addrinfo *host_info_list;
  const char *hostname = argv[1];
  const char *port = argv[2];
  memset(&host_info, 0, sizeof(host_info));
  host_info.ai_family = AF_UNSPEC;
  host_info.ai_socktype = SOCK_STREAM;

  status = getaddrinfo(hostname, port, &host_info, &host_info_list);
  if (status != 0) {
    std::cerr << "Error: cannot get address info for host" << std::endl;
    std::cerr << "  (" << hostname << "," << port << ")" << std::endl;
    return -1;
  } // if

  server_socket_fd =
      socket(host_info_list->ai_family, host_info_list->ai_socktype,
             host_info_list->ai_protocol);
  if (server_socket_fd == -1) {
    std::cerr << "Error: cannot create socket" << std::endl;
    std::cerr << "  (" << hostname << "," << port << ")" << std::endl;
    return -1;
  } // if

  // std::cout << "Connecting to " << hostname << " on port " << port << "..."
  //           << std::endl;

  status = connect(server_socket_fd, host_info_list->ai_addr,
                   host_info_list->ai_addrlen);
  if (status == -1) {
    std::cerr << "Error: cannot connect to server" << std::endl;
    std::cerr << "  (" << hostname << "," << port << ")" << std::endl;
    return -1;
  } // if
    // port_bind_num;

  std::string str_message = std::to_string(port_bind_num);
  char *message = new char[str_message.length() + 1];
  std::strcpy(message, str_message.c_str());
  send(server_socket_fd, message, strlen(message), 0); // Sending its own
  delete[] message;
  int self_index;
  int total_players;
  recv(server_socket_fd, &self_index, sizeof(long int), 0);
  recv(server_socket_fd, &total_players, sizeof(long int), 0);
  std::cout << "Connected as player " << self_index << " out of "
            << total_players << " total players" << std::endl;
  // This is after you connect to the ringmaster, and recv'ing who to connect
  // to.
  // sockaddr_in *connect_sock =
  //     new sockaddr_in; // dont forget to delete once handled
  // Receiving what to connect to
  // Send as IP and port?
  unsigned short received_port = 0;
  struct in_addr received_addr;
  int recv_port =
      recv(server_socket_fd, &received_port, sizeof(unsigned short), 0);
  int recv_addr =
      recv(server_socket_fd, &received_addr, sizeof(struct in_addr), 0);
  if (recv_port != sizeof(unsigned short) ||
      recv_addr != sizeof(struct in_addr)) {
    std::cout << "Did not receive the whole address and buffer" << std::endl;
  }
  srand((unsigned int)time(NULL) + self_index); // seed the random
  // How to implement if recv does not get all?, just call it again?

  // Now, generate new socket to connect with the portnum/ip given in the
  // connect_sock This time, like the player(not ringmaster).
  // Beginning of connect struct
  int status_connect;
  int socket_fd_connect;
  struct addrinfo host_info_connect;
  struct addrinfo *host_info_list_connect;
  // change these two to the values got from the master (from connect_sock)
  char ip4_addr[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &received_addr, ip4_addr, INET_ADDRSTRLEN);
  const char *hostname_connect = ip4_addr;
  // Now, port number
  std::string str_port = std::to_string(received_port);
  char *temp_connect = new char[str_port.length() + 1];
  std::strcpy(temp_connect, str_port.c_str());
  const char *port_connect = temp_connect;
  // std::cout << "Connecting to " << hostname_connect << "on " << port_connect
  //           << std::endl;

  //
  memset(&host_info_connect, 0, sizeof(host_info_connect));
  host_info_connect.ai_family = AF_UNSPEC;
  host_info_connect.ai_socktype = SOCK_STREAM;
  // delete connect_sock;
  status_connect = getaddrinfo(hostname_connect, port_connect,
                               &host_info_connect, &host_info_list_connect);
  if (status_connect != 0) {
    std::cerr << "Error: cannot get address info for host" << std::endl;
    std::cerr << "  (" << hostname_connect << "," << port_connect << ")"
              << std::endl;
    return -1;
  } // if
  socket_fd_connect = socket(host_info_list_connect->ai_family,
                             host_info_list_connect->ai_socktype,
                             host_info_list_connect->ai_protocol);
  if (socket_fd_connect == -1) {
    std::cerr << "Error: cannot create socket to connect" << std::endl;
    std::cerr << "  (" << hostname_connect << "," << port_connect << ")"
              << std::endl;
    return -1;
  } // if
  // Create socket, and wait before you connect.
  // std::cout << "Ready to listen twice" << std::endl;
  int left_neigh_fd;
  int right_neigh_fd;
  for (int i = 0; i < 2; i++) { // This needs to happen two times
    char recva[5];
    int server_check = recv(server_socket_fd, recva, 5, 0);
    // std::cout << "Received " << server_check << std::endl;
    // std::cout << "Num of bytes received " << server_check << " as "
    //           << sizeof(recva) << std::endl;
    if (server_check <= 0) {
      perror("This happened:");
      std::cout << "Size of received is" << server_check << std::endl;
    }
    std::string recv_string(recva, 3);
    std::string acc_string("Acc");
    // std::cout << "received from server " << recva << std::endl;
    // std::cout << recv_string << std::endl;
    if (!recv_string.compare(acc_string)) {
      // std::cout << "Told to Acc" << std::endl;
      // if 0, it is accept
      struct sockaddr_storage acc_socket_addr;
      socklen_t acc_socket_addr_len = sizeof(acc_socket_addr);
      // std::cout << "Trying to accept " << std::endl;
      right_neigh_fd =
          accept(socket_fd_listen, (struct sockaddr *)&acc_socket_addr,
                 &acc_socket_addr_len);
      // std::cout << "Accepted " << std::endl;
      if (right_neigh_fd == -1) {
        std::cout << "Error: Cannect accept connection of right neigh"
                  << std::endl;
      }
      char accept_success = 'a';
      // std::cout << "Tell I got the accepting working " << std::endl;
      send(server_socket_fd, &accept_success, sizeof(char), 0);
    } else {
      // std::cout << "Told to Con" << std::endl;
      // if it is 1, it is connect
      if (server_check <= 0) {
        perror("This happened:");
        std::cout << "Size of received is" << server_check << std::endl;
      }
      // std::cout << "Connecting to " << hostname_connect << " on port "
      //          << port_connect << "..." << std::endl;

      status_connect =
          connect(socket_fd_connect, host_info_list_connect->ai_addr,
                  host_info_list_connect->ai_addrlen);
      // std::cout << "Ok, connected" << std::endl;
      left_neigh_fd = socket_fd_connect;
      if (left_neigh_fd == -1) {
        std::cerr << "Error: cannot connect to socket" << std::endl;
        std::cerr << "  (" << hostname_connect << "," << port_connect << ")"
                  << std::endl;
        // std::cout << "Tell I got the connecting working " << std::endl;

      } // if
      char connect_success = 'c';
      send(server_socket_fd, &connect_success, sizeof(char), 0);
    }
  }
  int left_neigh_index = self_index - 1;
  if (self_index == 0) {
    left_neigh_index = total_players - 1;
  }
  int right_neigh_index = self_index + 1;
  if (self_index == total_players - 1) {
    right_neigh_index = 0;
  }
  // To listen
  int select_n = right_neigh_fd + 1;
  tv.tv_sec = 20;
  tv.tv_usec = 500000;
  // Set up indices, now wait on all three sockets.
  // Set up num_hops to 513 now, as an invalid way. Once you receive, change it
  // to what you get first.
  long int num_hops = 513;
  std::vector<int> temp_vec;
  int size_message;
  // std::cout << "Size of int is" << sizeof(int) << std::endl;
  while (num_hops > 0) {
    FD_SET(right_neigh_fd, &readfds);
    FD_SET(server_socket_fd, &readfds);
    FD_SET(left_neigh_fd, &readfds);
    // std::cout << "Waiting to select..." << std::endl;
    int rv = select(select_n, &readfds, NULL, NULL, &tv);
    // std::cout << "After select" << std::endl;
    std::vector<int> vec_temp;
    if (rv == -1) {
      std::cout << "Error" << std::endl;
    } else if (rv == 0) {
      std::cout << "Timeout" << std::endl;
    } else {
      if (FD_ISSET(left_neigh_fd, &readfds)) {
        if (recv(left_neigh_fd, &num_hops, sizeof(long int), 0) !=
            sizeof(long int)) {
          //?Try to break, as it only happens when disconnections happen anyway?
          break;
        }
        if (recv(left_neigh_fd, &size_message, sizeof(int), 0) != sizeof(int)) {
        }
        vec_temp.resize(size_message / sizeof(int));

        recv(left_neigh_fd, &vec_temp.data()[0], size_message, MSG_WAITALL);

      } else if (FD_ISSET(server_socket_fd, &readfds)) {
        recv(server_socket_fd, &num_hops, sizeof(long int), 0);
        if (num_hops == -1) { // To identify it is being closed. Server won't
          break;
        }
      } else if (FD_ISSET(right_neigh_fd, &readfds)) {
        // std::cout << "Received from right neigh" << std::endl;
        if (recv(right_neigh_fd, &num_hops, sizeof(long int), 0) !=
            sizeof(long int)) {
          break;
        }
        if (recv(right_neigh_fd, &size_message, sizeof(int), 0) !=
            sizeof(int)) {
          // std::cout << "Problem with size_message recv from neigh" <<
          // std::endl;
        }
        vec_temp.resize(size_message / sizeof(int));
        recv(right_neigh_fd, &vec_temp.data()[0], size_message, MSG_WAITALL);
      }
    }
    num_hops--; // Decrement num of hops.
    // Also, push_back to the temp_vec at hand
    vec_temp.push_back(
        self_index); // This way, just add that you have received it
    size_message = vec_temp.size() * sizeof(int);
    if (num_hops == 0) { // get out of the loop atm
      std::cout << "I'm it" << std::endl;
      send(server_socket_fd, &size_message, sizeof(int), 0);
      send(server_socket_fd, &vec_temp.data()[0], size_message, 0);
      long int closing_num;
      recv(server_socket_fd, &closing_num, sizeof(long int), 0);
      break;
    }

    int who_to_send = rand() % 2; // Can be either 0, or 1
    if (who_to_send == 0) {
      std::cout << "Sending potato to " << left_neigh_index << std::endl;
      send(left_neigh_fd, &num_hops, sizeof(long int), 0);
      send(left_neigh_fd, &size_message, sizeof(int), 0);
      int should_be_sent = size_message;
      int indexing = 0;
      while (true) {
        int actual_sent =
            send(left_neigh_fd, &vec_temp.data()[indexing], should_be_sent, 0);
        should_be_sent -= actual_sent;
        indexing += actual_sent;
        if (should_be_sent == 0) {
          break;
        }
        // Again, send 3 things
      }
    } else {
      std::cout << "Sending potato to " << right_neigh_index << std::endl;
      send(right_neigh_fd, &num_hops, sizeof(long int), 0);
      send(right_neigh_fd, &size_message, sizeof(int), 0);
      // send(right_neigh_fd, &vec_temp.data()[0], size_message, 0);

      // std::cout << "Can't send vector properlly" << std::endl;
      int should_be_sent = size_message;
      int indexing = 0;
      while (true) {
        int actual_sent =
            send(right_neigh_fd, &vec_temp.data()[indexing], should_be_sent, 0);
        should_be_sent -= actual_sent;
        indexing += actual_sent;
        if (should_be_sent == 0) {
          break;
        }
        // Again, send 3 things
      }
    }
    // If not 0 yet, just send it to someone else
    FD_ZERO(&readfds);
  }

  // End of connect struct
  freeaddrinfo(host_info_list);
  freeaddrinfo(host_info_list_listen);
  freeaddrinfo(host_info_list_connect);
  close(server_socket_fd);
  close(right_neigh_fd);
  close(left_neigh_fd);
  delete[] temp_connect;
  return EXIT_SUCCESS;
}

// recv(MSG_WAITALL)
