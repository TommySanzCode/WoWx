#pragma once
#include "core/coordinates.hpp"
#include <glm/gtc/matrix_transform.hpp>
// Match pinned WoWee core/world_loader.cpp's WDT-only placement convention.
// Zero-position instances use server-relative coordinates, without ADT origin.
inline glm::mat4 globalWorldPlacement(const float* pos,const float* rot){
    glm::vec3 translation(0);
    if(pos[0]!=0||pos[1]!=0||pos[2]!=0)translation=wowee::core::coords::adtToWorld(pos[0],pos[1],pos[2]);
    auto matrix=glm::translate(glm::mat4(1),translation);
    matrix=glm::rotate(matrix,glm::radians(rot[1]+180),glm::vec3(0,0,1));
    matrix=glm::rotate(matrix,glm::radians(-rot[0]),glm::vec3(0,1,0));
    return glm::rotate(matrix,glm::radians(-rot[2]),glm::vec3(1,0,0));
}
