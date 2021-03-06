// the extent server implementation

#include "extent_server.h"
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

extent_server::extent_server() {
  im = new inode_manager();
}

// int extent_server::init_extent_server(unsigned short uid, unsigned short gid, extent_protocol::extentid_t &id) 
// {
//   printf("in init extent server\n");
//   this->uid = uid;
//   this->gid = gid;
//   return extent_protocol::OK;
// }

int extent_server::create(uint32_t type, extent_protocol::AccessControl ac, mode_t mode, extent_protocol::extentid_t &id) {
  printf("extent_server: create inode\n");
  printf("mode:%o, uid:%d - gid%d\n", mode, ac.uid, ac.gid);
  id = im->alloc_inode(type, ac.uid, ac.gid, mode);
  return extent_protocol::OK;
}

int extent_server::setattr(extent_protocol::attr a, extent_protocol::extentid_t id, int &t) {
  im->setattr(id, a);
  return extent_protocol::OK;
}

int extent_server::put(extent_protocol::extentid_t id, std::string buf, int &)
{
  id &= 0x7fffffff;
  
  const char * cbuf = buf.c_str();
  int size = buf.size();
  im->write_file(id, cbuf, size);
  return extent_protocol::OK;
}

int extent_server::get(extent_protocol::extentid_t id, std::string &buf)
{
  printf("extent_server: get %lld\n", id);

  id &= 0x7fffffff;

  int size = 0;
  char *cbuf = NULL;

  im->read_file(id, &cbuf, &size);
  if (size == 0)
    buf = "";
  else {
    buf.assign(cbuf, size);
    free(cbuf);
  }

  return extent_protocol::OK;
}

int extent_server::getattr(extent_protocol::extentid_t id, extent_protocol::attr &a)
{
  printf("extent_server: getattr %lld\n", id);

  id &= 0x7fffffff;
  
  extent_protocol::attr attr;
  memset(&attr, 0, sizeof(attr));
  im->getattr(id, attr);
  a = attr;
  printf("uid:%d - gid:%d\n", a.uid, a.gid);

  return extent_protocol::OK;
}

int extent_server::remove(extent_protocol::extentid_t id, int &)
{
  printf("extent_server: write %lld\n", id);

  id &= 0x7fffffff;
  im->remove_file(id);
 
  return extent_protocol::OK;
}

