// Ray tracing a triangle mesh (Voyager 2) through its BVH (TriangleBvh.cpp).
// Included by raytrace.glsl. Positions are camera-relative in world space;
// rays are moved into the mesh's local frame, where the tree was built.
//
// Buffer layouts (RGBA32F texels):
//   bvhNodes,     2 per node:     (min.xyz, first)  (max.xyz, second)
//                 second < 0: leaf holding -second triangles from `first`
//                 second >= 0: internal node with children first, second
//   bvhTriangles, 7 per triangle: (p0, material) (p1, -) (p2, -)
//                 (n0, u0) (n1, v0) (n2, u1) (v1, u2, v2, -)

#define MAX_MESH_MATERIALS 16
#define BVH_STACK 32

uniform int meshEnabled;
uniform samplerBuffer bvhNodes;
uniform samplerBuffer bvhTriangles;
uniform vec3 meshPosition;          // camera-relative
uniform mat3 meshWorldToLocal;      // inverse rotation
uniform float meshBoundingRadius;

struct MeshHit
{
	float t;          // -1 on a miss
	int triangle;
	vec2 barycentric; // weights of p1 and p2 (p0 gets 1 - u - v)
};

// Slab test: the ray is inside the box on all three axes over [tNear, tFar].
bool intersectBox(vec3 origin, vec3 inverseDirection, vec3 boxMin, vec3 boxMax, float tMax)
{
	vec3 t0 = (boxMin - origin) * inverseDirection;
	vec3 t1 = (boxMax - origin) * inverseDirection;
	vec3 tSmall = min(t0, t1);
	vec3 tBig = max(t0, t1);
	float tNear = max(max(tSmall.x, tSmall.y), tSmall.z);
	float tFar = min(min(tBig.x, tBig.y), tBig.z);
	return tFar >= max(tNear, 0.0) && tNear < tMax;
}

// Moller-Trumbore: solve origin + t d = p0 + u (p1 - p0) + v (p2 - p0) with
// Cramer's rule. Returns t, or -1 if the ray misses the triangle.
float intersectTriangle(vec3 origin, vec3 direction, vec3 p0, vec3 p1, vec3 p2, out vec2 barycentric)
{
	vec3 edge1 = p1 - p0;
	vec3 edge2 = p2 - p0;
	vec3 pVector = cross(direction, edge2);
	float determinant = dot(edge1, pVector);
	if (abs(determinant) < 1e-20)
		return -1.0;                      // ray parallel to the triangle
	float inverseDeterminant = 1.0 / determinant;
	vec3 tVector = origin - p0;
	float u = dot(tVector, pVector) * inverseDeterminant;
	if (u < 0.0 || u > 1.0)
		return -1.0;
	vec3 qVector = cross(tVector, edge1);
	float v = dot(direction, qVector) * inverseDeterminant;
	if (v < 0.0 || u + v > 1.0)
		return -1.0;
	barycentric = vec2(u, v);
	return dot(edge2, qVector) * inverseDeterminant;
}

// Nearest hit in (tMin, tMax). anyHit = true stops at the first hit found,
// which is all a shadow ray needs.
MeshHit intersectMesh(vec3 worldOrigin, vec3 worldDirection, float tMin, float tMax, bool anyHit)
{
	MeshHit hit;
	hit.t = -1.0;
	hit.triangle = -1;
	hit.barycentric = vec2(0.0);
	if (meshEnabled == 0)
		return hit;

	// Cheap reject: the ray must pass through the mesh's bounding sphere.
	vec3 offset = worldOrigin - meshPosition;
	float b = dot(offset, worldDirection);
	float c = dot(offset, offset) - meshBoundingRadius * meshBoundingRadius;
	if ((c > 0.0 && b > 0.0) || b * b - c < 0.0)
		return hit;

	// Into the mesh frame (a pure rotation keeps t unchanged).
	vec3 origin = meshWorldToLocal * offset;
	vec3 direction = meshWorldToLocal * worldDirection;
	vec3 inverseDirection = 1.0 / (direction + vec3(1e-20));

	int stack[BVH_STACK];
	int stackSize = 0;
	stack[stackSize++] = 0;
	float closest = tMax;
	while (stackSize > 0)
	{
		int node = stack[--stackSize];
		vec4 lower = texelFetch(bvhNodes, node * 2);
		vec4 upper = texelFetch(bvhNodes, node * 2 + 1);
		if (!intersectBox(origin, inverseDirection, lower.xyz, upper.xyz, closest))
			continue;

		int first = int(lower.w);
		int second = int(upper.w);
		if (second < 0)
		{
			for (int k = 0; k < -second; ++k)
			{
				int triangle = first + k;
				vec3 p0 = texelFetch(bvhTriangles, triangle * 7).xyz;
				vec3 p1 = texelFetch(bvhTriangles, triangle * 7 + 1).xyz;
				vec3 p2 = texelFetch(bvhTriangles, triangle * 7 + 2).xyz;
				vec2 barycentric;
				float t = intersectTriangle(origin, direction, p0, p1, p2, barycentric);
				if (t > tMin && t < closest)
				{
					closest = t;
					hit.triangle = triangle;
					hit.barycentric = barycentric;
					if (anyHit)
					{
						hit.t = t;
						return hit;
					}
				}
			}
		}
		else if (stackSize < BVH_STACK - 1)
		{
			stack[stackSize++] = first;
			stack[stackSize++] = second;
		}
	}
	if (hit.triangle >= 0)
		hit.t = closest;
	return hit;
}

// Interpolated surface data at a hit, back in world orientation.
void meshSurface(MeshHit hit, out vec3 worldNormal, out vec2 uv, out int material)
{
	int base = hit.triangle * 7;
	vec4 t0 = texelFetch(bvhTriangles, base);
	vec4 t3 = texelFetch(bvhTriangles, base + 3);
	vec4 t4 = texelFetch(bvhTriangles, base + 4);
	vec4 t5 = texelFetch(bvhTriangles, base + 5);
	vec4 t6 = texelFetch(bvhTriangles, base + 6);
	float w0 = 1.0 - hit.barycentric.x - hit.barycentric.y;
	vec3 localNormal = normalize(t3.xyz * w0 + t4.xyz * hit.barycentric.x + t5.xyz * hit.barycentric.y);
	worldNormal = normalize(transpose(meshWorldToLocal) * localNormal);
	uv = vec2(t3.w, t4.w) * w0 + vec2(t5.w, t6.x) * hit.barycentric.x + t6.yz * hit.barycentric.y;
	material = int(t0.w);
}
