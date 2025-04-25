#ifndef OBJ_H
#define OBJ_H

#include "rapidobj.hpp"

class ObjLoader
{
public:
  static rapidobj::Result load_obj(int argc, char **argv);
};


#endif /* OBJ_H */
