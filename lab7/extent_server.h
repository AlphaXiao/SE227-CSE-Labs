// this is the extent server

#ifndef extent_server_h
#define extent_server_h

#include <string>
#include <map>
#include "extent_protocol.h"
#include "inode_manager.h"

class extent_server {
 protected:
#if 0
  typedef struct extent {
    std::string data;
    struct extent_protocol::attr attr;
  } extent_t;
  std::map <extent_protocol::extentid_t, extent_t> extents;
#endif
  inode_manager *im;

private:
  // unsigned short uid;
  // unsigned short gid;

 public:
  extent_server();
  // int init_extent_server(unsigned short uid, unsigned short gid, extent_protocol::extentid_t &id);
  int create(uint32_t type, extent_protocol::AccessControl ac, mode_t mode, extent_protocol::extentid_t &id);
  int put(extent_protocol::extentid_t id, std::string, int &);
  int get(extent_protocol::extentid_t id, std::string &);
  int getattr(extent_protocol::extentid_t id, extent_protocol::attr &);
  int setattr( extent_protocol::attr , extent_protocol::extentid_t id, int &);
  int remove(extent_protocol::extentid_t id, int &);
};

#endif 







