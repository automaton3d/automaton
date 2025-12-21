#pragma once

#ifndef __CUDACC__
  #include <glm/glm.hpp>
  #include <glm/gtc/matrix_transform.hpp>
  #include <glm/gtc/type_ptr.hpp>
  #include <glm/gtc/matrix_inverse.hpp>
#else
  #error "GLM header included in CUDA compilation unit"
#endif
