#ifndef OBJ_H
#define OBJ_H

#include "mesh.hpp"

class ObjLoader
{
public:
  static MeshSettings load_obj(int argc, char **argv);
};


#endif /* OBJ_H */
