// Definition of the Socket class

#ifndef Socket_class
#define Socket_class


#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <string>
#include <arpa/inet.h>


const int MAXHOSTNAME = 200;
const int MAXCONNECTIONS = 5;
const int MAXRECV = 500;

class Socket
{
 public:
  Socket();
  virtual ~Socket();

  // Server initialization
  bool create();
  bool bind ( const int port );
  bool listen() const;
  virtual Socket* accept( Socket* alreadyCreated=NULL ) const;

  // Client initialization
  bool connect ( const std::string host, const int port );

  // Data Transimission
  bool send ( const std::string& s ) const;
  bool send ( const unsigned char& c ) const;
  int recv ( std::string&, const int& max=MAXRECV ) const;

  const Socket& operator << ( const std::string& ) const;
  const Socket& operator << ( const unsigned char& c ) const;
  const Socket& operator >> ( std::string& ) const;
  const Socket& operator >> ( unsigned char& ) const;
  
  std::string addressAsString();
  
  void set_non_blocking ( const bool );

  bool is_valid() const;

 private:
  int m_sock;
  sockaddr_in m_addr;


};


#endif
