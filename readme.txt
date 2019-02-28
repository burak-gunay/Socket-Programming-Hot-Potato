What the ringmaster call to set up;
./ringmaster <port_num> <num_players> <num_hops>
For example
./ringmaster 4444 5 10
If the port number is invalid(or there are other problems with the setting up the socket), the process will terminate.

What the player call to set up;
./player <machine_name> <port_num>
Machine name is either the IP address or the hostname of the machine that the ringmaster is running on. Port number is 
the port number the ringmasters listens on. 

You can run this code in the same VM with different tabs, or run it from different computers as well.
Have fun!
