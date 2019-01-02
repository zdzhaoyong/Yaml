#include "YAML.h"
#include <iostream>
#include <fstream>
#include <sstream>

int main ()
{
  std::stringstream context;
  std::ifstream ifs("/data/zhaoyong/Program/Thirdparty/handsfree/rplidar_ros/launch/yaml/rplidar/laser_filter_stone_v3.yaml");
//  std::ifstream ifs("/data/zhaoyong/Program/Apps/Tests/js-yaml/examples/list.nested.yml");
  std::string line;
  while(std::getline(ifs,line)){
     context<<line<<std::endl;
  }

  YAML yaml(context.str());
  Svar var=yaml.parse();
  std::cout<<var;

  return 0;
}
