#pragma once

#include "intersections.h"

// CHECKITOUT
/**
 * Computes a cosine-weighted random direction in a hemisphere.
 * Used for diffuse lighting.
 */
__host__ __device__
glm::vec3 calculateRandomDirectionInHemisphere(
        glm::vec3 normal, thrust::default_random_engine &rng) {
    thrust::uniform_real_distribution<float> u01(0, 1);

    float up = sqrt(u01(rng)); // cos(theta)
    float over = sqrt(1 - up * up); // sin(theta)
    float around = u01(rng) * TWO_PI;

    // Find a direction that is not the normal based off of whether or not the
    // normal's components are all equal to sqrt(1/3) or whether or not at
    // least one component is less than sqrt(1/3). Learned this trick from
    // Peter Kutz.

    glm::vec3 directionNotNormal;
    if (abs(normal.x) < SQRT_OF_ONE_THIRD) {
        directionNotNormal = glm::vec3(1, 0, 0);
    } else if (abs(normal.y) < SQRT_OF_ONE_THIRD) {
        directionNotNormal = glm::vec3(0, 1, 0);
    } else {
        directionNotNormal = glm::vec3(0, 0, 1);
    }

    // Use not-normal direction to generate two perpendicular directions
    glm::vec3 perpendicularDirection1 =
        glm::normalize(glm::cross(normal, directionNotNormal));
    glm::vec3 perpendicularDirection2 =
        glm::normalize(glm::cross(normal, perpendicularDirection1));

    return up * normal
        + cos(around) * over * perpendicularDirection1
        + sin(around) * over * perpendicularDirection2;
}

/**
 * Scatter a ray with some probabilities according to the material properties.
 * For example, a diffuse surface scatters in a cosine-weighted hemisphere.
 * A perfect specular surface scatters in the reflected ray direction.
 * In order to apply multiple effects to one surface, probabilistically choose
 * between them.
 * 
 * The visual effect you want is to straight-up add the diffuse and specular
 * components. You can do this in a few ways. This logic also applies to
 * combining other types of materias (such as refractive).
 * 
 * - Always take an even (50/50) split between a each effect (a diffuse bounce
 *   and a specular bounce), but divide the resulting color of either branch
 *   by its probability (0.5), to counteract the chance (0.5) of the branch
 *   being taken.
 *   - This way is inefficient, but serves as a good starting point - it
 *     converges slowly, especially for pure-diffuse or pure-specular.
 * - Pick the split based on the intensity of each material color, and divide
 *   branch result by that branch's probability (whatever probability you use).
 *
 * This method applies its changes to the Ray parameter `ray` in place.
 * It also modifies the color `color` of the ray in place.
 *
 * You may need to change the parameter list for your purposes!
 */


__host__ __device__
void scatterRay(
		PathSegment & pathSegment,
        glm::vec3 intersect,
        glm::vec3 normal,
        const Material &m,
        thrust::default_random_engine &rng) {
    // TODO: implement this.
    // A basic implementation of pure-diffuse shading will just call the
    // calculateRandomDirectionInHemisphere defined above.

	glm::vec3 newDirection;
	thrust::uniform_real_distribution<float> u01(0, 1);
	float dist = u01(rng);
	// If type==1, then it is diffuse
	if (dist < m.hasReflective) {
		newDirection = glm::reflect(pathSegment.ray.direction, normal);
		pathSegment.color *= m.specular.color;
	}
		
	else if (dist < m.hasRefractive + m.hasReflective) {


		thrust::uniform_real_distribution<float> u01(0, 1);

		glm::vec3 normalRefract =  normal;
		float r0 = powf(1- m.indexOfRefraction / (1 + m.indexOfRefraction), 2.0f);
		float r1 = r0 + (1 - r0)*powf(1 - (glm::dot(glm::normalize(pathSegment.ray.direction), normalRefract)), 5.0f);
		float eta = m.indexOfRefraction;
		bool inward = glm::dot(glm::normalize(pathSegment.ray.direction), normalRefract) < 0.0f;
		if (u01(rng) >  r1)
			newDirection = glm::reflect(pathSegment.ray.direction, normalRefract);
		else
		{
			normalRefract = inward ? normalRefract : -10.f * normalRefract;
			eta = inward ? 1 / eta : eta;
			newDirection = glm::refract(pathSegment.ray.direction, normalRefract, eta);
			if (glm::length(newDirection) < 0.01f) {
				pathSegment.color *= 0;
				newDirection = glm::reflect(pathSegment.ray.direction, normalRefract);
			}
		}

		/*
		glm::vec3 normalRefract = 1.0f * normal;
		float eta = 1/m.indexOfRefraction;
		float r0,r1;
		bool mediaAirToMaterial = glm::dot(pathSegment.ray.direction, normal) < 0.0f;
		r0 = 1.0f- m.indexOfRefraction;
		if (mediaAirToMaterial) {
			normalRefract = -1.0f * normalRefract;
			eta = 1 / eta;
			r0 = m.indexOfRefraction - 1.0f;
		}
		

		if (glm::length(newDirection) < 0.01f) {
			pathSegment.color *= 0;
			newDirection = glm::reflect(pathSegment.ray.direction, normalRefract);
		}
		r0 = powf(r0 / (1 + m.indexOfRefraction), 2.0f);
		r1 = r0 + (1 - r0)*powf(1-(glm::dot(glm::normalize(pathSegment.ray.direction), normalRefract)),5.0f);
		
		if (r1 < u01(rng))
			newDirection = glm::reflect(pathSegment.ray.direction, normalRefract);
		else
			newDirection = glm::refract(pathSegment.ray.direction, normalRefract, eta);
			*/



		pathSegment.color *= m.specular.color;
	}
	else {
		newDirection = calculateRandomDirectionInHemisphere(normal, rng);
		pathSegment.color = pathSegment.color * m.color;
	}
		

	/*
	// If the object is refractive
	else if (type == 3) {
		
		if (glm::dot(incident, normal) > 0) 
			eta = 1/eta;


		newDirection = glm::refract(incident, normal, eta);
	}
	*/
	//newDirection = calculateRandomDirectionInHemisphere(normal, rng);
 	pathSegment.ray.direction = newDirection;
	pathSegment.ray.origin = intersect + newDirection*0.01f;
	
}
