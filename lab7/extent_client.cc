// RPC stubs for clients to talk to extent_server

#include "extent_client.h"
#include <sstream>
#include <fstream>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <time.h>

extent_client::extent_client(std::string dst, std::string name, unsigned short uid, unsigned short gid)
{
  sockaddr_in dstsock;
  ac = extent_protocol::AccessControl(uid, gid);
  username = name;
  make_sockaddr(dst.c_str(), &dstsock);
  cl = new rpcc(dstsock);
  if (cl->bind() != 0) {
    printf("extent_client: bind failed\n");
  }

  // int id;
  // int ret = cl->call(extent_protocol::init, uid, gid, id);
}

extent_protocol::status
extent_client::getattr(extent_protocol::extentid_t eid, 
		       extent_protocol::attr &attr)
{
  extent_protocol::status ret = extent_protocol::OK;
  ret = cl->call(extent_protocol::getattr, eid, attr);
  return ret;
}

extent_protocol::status
extent_client::create(uint32_t type, extent_protocol::extentid_t &id, mode_t mode)
{
  extent_protocol::status ret = extent_protocol::OK;
  ret = cl->call(extent_protocol::create, type, ac, mode, id);
  return ret;
}


extent_protocol::status 
extent_client::setattr(extent_protocol::extentid_t eid, 
                                  extent_protocol::attr &a) {
    extent_protocol::status ret = extent_protocol::OK;
    
    extent_protocol::attr attr;
    ret = getattr(eid, attr);
    printf("extent_client::setattr:gid-%d, uid:%d\n", attr.gid, attr.uid);
    if(attr.uid != ac.uid) {
          return extent_protocol::NOPEM;
    }     

    ret = cl->call(extent_protocol::setattr, eid, a);
    return ret;
}


extent_protocol::status
extent_client::get(extent_protocol::extentid_t eid, std::string &buf, bool forwrite)
{
  extent_protocol::status ret = extent_protocol::OK;
  // Your lab3 code goes here
  extent_protocol::attr attr;
  ret = getattr(eid, attr);
  printf("extent_client::get:gid-%d, uid:%d\n", attr.gid, attr.uid);
  if(attr.gid != 777) {
      int g = getGroup(attr.uid, attr.gid);
      printf("group number:%d\n", g);
      printf("mode:%04o\n", attr.mode);
      // check permission
      int m = (attr.mode>>(g*3)) & (4);
      if( m == 0 && forwrite == false) {
        printf("error\n");
        return extent_protocol::NOPEM;
      }
  }
  ret = cl->call(extent_protocol::get, eid, buf);
  return ret;
}

extent_protocol::status
extent_client::put(extent_protocol::extentid_t eid, std::string buf)
{
  extent_protocol::status ret = extent_protocol::OK;
  
  extent_protocol::attr attr;
  ret = getattr(eid, attr);
  printf("extent_client::put:gid-%d, uid:%d\n", attr.gid, attr.uid);
  if(attr.gid != 777) {
      int g = getGroup(attr.uid, attr.gid);
      printf("group number:%d\n", g);
      printf("mode:%04o\n", attr.mode);
      // check permission
      int m = (attr.mode>>(g*3)) & (2);
      if( m == 0 ) {
        printf("error write\n");
        return extent_protocol::NOPEM;
      }
  }

  int t;
  ret = cl->call(extent_protocol::put, eid, buf, t);
  return ret;
}

extent_protocol::status
extent_client::remove(extent_protocol::extentid_t eid)
{
  extent_protocol::status ret = extent_protocol::OK;

  extent_protocol::attr attr;
  ret = getattr(eid, attr);
  printf("extent_client::put:gid-%d, uid:%d\n", attr.gid, attr.uid);
  if(attr.gid != 777) {
      int g = getGroup(attr.uid, attr.gid);
      printf("group number:%d\n", g);
      printf("mode:%04o\n", attr.mode);
      // check permission
      int m = (attr.mode>>(g*3)) & (2);
      if( m == 0 ) {
        printf("error write\n");
        return extent_protocol::NOPEM;
      }
  }


  int t;
  ret = cl->call(extent_protocol::remove, eid, t);
  return ret;
}



int extent_client::getGroup(unsigned int uid, unsigned int gid) {
    if(uid == ac.uid) {
        return 2;
    } 
    std::ifstream in("./etc/group");
    std::string buf;
    while(getline(in, buf)) {
        extent_protocol::GroupInfo info = extent_protocol::parseGroup(buf);
        if(gid == info.gid) {
            for(int i = 0; i < info.members.size(); ++i) {
                if(username == info.members[i]) {
                    return 1;
                }
            }
            break;
        }
    }
    return 0;
}
