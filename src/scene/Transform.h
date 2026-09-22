#ifndef TRANSFORM_H
#define TRANSFORM_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

// Reusable transform for every scene object.
// Physical state uses double precision (see the implementation bible, section 12);
// a float model matrix is produced only at render time.
struct Transform
{
	glm::dvec3 position{ 0.0 };
	glm::dquat rotation{ 1.0, 0.0, 0.0, 0.0 };
	glm::dvec3 scale{ 1.0 };

	// Local transform in double precision.
	glm::dmat4 localMatrix() const
	{
		const glm::dmat4 translation = glm::translate(glm::dmat4(1.0), position);
		const glm::dmat4 orientation = glm::mat4_cast(rotation);
		const glm::dmat4 scaling = glm::scale(glm::dmat4(1.0), scale);
		return translation * orientation * scaling;
	}

	// Float model matrix for GPU upload.
	glm::mat4 modelMatrix() const
	{
		return glm::mat4(localMatrix());
	}
};

#endif
