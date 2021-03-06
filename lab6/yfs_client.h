#ifndef yfs_client_h
#define yfs_client_h

#include <string>
#include "lock_protocol.h"
#include "lock_client.h"
//#include "yfs_protocol.h"
#include "extent_client.h"
#include <vector>


class yfs_client {
  extent_client *ec;
  lock_client *lc;

 public:

  typedef unsigned long long inum;
  enum xxstatus { OK, RPCERR, NOENT, IOERR, EXIST };
  typedef int status;

  struct fileinfo {
    unsigned long long size;
    unsigned long atime;
    unsigned long mtime;
    unsigned long ctime;
  };
  struct dirinfo {
    unsigned long atime;
    unsigned long mtime;
    unsigned long ctime;
  };

  // struct direntItem
  // {
  //   char name[32-sizeof(yfs_client::inum)];
  //   yfs_client::inum inum;
  // };

  struct dirent {
    std::string name;
    yfs_client::inum inum;
  };

 private:
  static std::string filename(inum);
  static inum n2i(std::string);
  void appendDirContent(std::string &, const char *, inum &);
  void appendDirContent(std::string &, std::string &, inum &);
  int _lookup(inum, const char *, bool &, inum &);
  int _readdir(inum, std::list<dirent> &);
  int _getfile(inum, fileinfo &);  
  
 public:
  yfs_client(std::string, std::string);
  
 public:
  // yfs_client();
  yfs_client(std::string);

  bool isfile(inum);
  bool isdir(inum);

  int getfile(inum, fileinfo &);
  int getdir(inum, dirinfo &);

  int setattr(inum, size_t);

  int setattr_atime(inum, unsigned long long);
  int lookup(inum, const char *, bool &, inum &);
  int create(inum, const char *, mode_t, inum &);
  int symlink(inum, const char *, const char *, inum&);

  int readdir(inum, std::list<dirent> &);
  int clear(inum);
  int write(inum, size_t, off_t, const char *, size_t &);
  int read(inum, size_t, off_t, std::string &);
  int unlink(inum,const char *);
  int mkdir(inum , const char *, mode_t , inum &);
};

#endif 
