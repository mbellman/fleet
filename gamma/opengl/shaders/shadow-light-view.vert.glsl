#version 460 core

uniform mat4 matLightViewProjection;

layout (location = 0) in vec3 vertexPosition;
layout (location = 1) in vec3 vertexNormal;
layout (location = 2) in vec3 vertexTangent;
layout (location = 3) in vec2 vertexUv;
layout (location = 4) in uint modelColor;
layout (location = 5) in mat4 modelMatrix;

out vec2 fragUv;

#include "utils/gl.glsl";
#include "utils/preset-animation.glsl";

void main() {
  // @hack invert Z
  vec4 world_position = glVec4(modelMatrix * vec4(vertexPosition, 1.0));

  // @todo make a utility for this
  switch (animation.type) {
    case FLOWER:
      world_position.xyz += getFlowerAnimationOffset(vertexPosition, world_position.xyz);
      break;
    case LEAF:
      world_position.xyz += getLeafAnimationOffset(vertexPosition, world_position.xyz);
      break;
    case BIRD:
      world_position.xyz += getBirdAnimationOffset(vertexPosition, world_position.xyz);
      break;
  }

  gl_Position = matLightViewProjection * world_position;

  fragUv = vertexUv;
}