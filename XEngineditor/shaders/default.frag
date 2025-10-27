	#version 460 core
	out vec4 FragColor;

	uniform float randSeed;
	uniform vec2 u_resolution;
	uniform vec4 spheres[16]; // 最多 8 顆球，每顆球用兩個 vec4
	uniform int sphereCount;
	uniform vec3 cameraPos;     // lookfrom
	uniform vec3 cameraTarget;  // lookat
	uniform float cameraFov;
	uniform float samples_per_pixel;
	uniform vec3 backgroundColor;
	uniform int max_depth;

	const float EPSILON = 1e-4;

	vec2 get_uv(vec2 fragCoord, vec2 jitter, vec2 resolution) {
		vec2 uv = (fragCoord + jitter) / resolution;
		uv = uv * 2.0 - 1.0;
		uv.x *= resolution.x / resolution.y;
		return uv;
	}

	uint hash( uint x ) {
		x += ( x << 10u );
		x ^= ( x >>  6u );
		x += ( x <<  3u );
		x ^= ( x >> 11u );
		x += ( x << 15u );
		return x;
	}


	// Compound versions of the hashing algorithm I whipped together.
	uint hash( uvec2 v ) { return hash( v.x ^ hash(v.y)                         ); }
	uint hash( uvec3 v ) { return hash( v.x ^ hash(v.y) ^ hash(v.z)             ); }
	uint hash( uvec4 v ) { return hash( v.x ^ hash(v.y) ^ hash(v.z) ^ hash(v.w) ); }

	float floatConstruct( uint m ) {
		const uint ieeeMantissa = 0x007FFFFFu; // binary32 mantissa bitmask
		const uint ieeeOne      = 0x3F800000u; // 1.0 in IEEE binary32

		m &= ieeeMantissa;                     // Keep only mantissa bits (fractional part)
		m |= ieeeOne;                          // Add fractional part to 1.0

		float  f = uintBitsToFloat( m );       // Range [1:2]
		return f - 1.0;                        // Range [0:1]
	}

	float random( float x ) { return floatConstruct(hash(floatBitsToUint(x))); }
	float random( vec2  v ) { return floatConstruct(hash(floatBitsToUint(v))); }
	float random( vec3  v ) { return floatConstruct(hash(floatBitsToUint(v))); }
	float random( vec4  v ) { return floatConstruct(hash(floatBitsToUint(v))); }


	float random_float(vec2 p) {
		return random(p);
	}

	vec2 random_vec2(vec2 p) {
		return vec2(random_float(gl_FragCoord.xy + p), random_float(gl_FragCoord.yx + vec2(1.0, 0.0)));
	}

	vec3 random_color(vec2 co) {
		return vec3(random_float(co + vec2(1.0, 0.0)),
					random_float(co + vec2(0.0, 1.0)),
					random_float(co + vec2(1.0, 1.0)));
	}

	vec3 random_unit_vector(vec2 co) {
		float z = random_float(co) * 2.0 - 1.0; // z in [-1, 1]
		float t = random_float(co + vec2(1.0, 1.0)) * 6.28318530718; // t in [0, 2π]
		float r = sqrt(1.0 - z * z); // radius at z

		return vec3(r * cos(t), r * sin(t), z);
	}

	vec3 random_in_hemisphere(vec3 normal, vec2 seed) {
		vec3 in_unit = random_unit_vector(seed);
		return dot(in_unit, normal) > 0.0 ? in_unit : -in_unit;
	}

	struct HitRecord {
		float t;
		int idx;
		vec3 pos;
		vec3 normal;
		vec3 color;
	};

	float hit_sphere(vec3 center, float radius, vec3 rayOrigin, vec3 rayDir) {
		vec3 oc = rayOrigin - center;
		float b = 2.0 * dot(oc, rayDir);
		float c = dot(oc, oc) - radius * radius;
		float discriminant = b*b - 4.0*c;
		if (discriminant < 0.0) return -1.0;
		float sqrtD = sqrt(discriminant);
		float t1 = (-b - sqrtD) * 0.5;
		float t2 = (-b + sqrtD) * 0.5;
		if (t1 > EPSILON) return t1;
		if (t2 > EPSILON) return t2;
		return -1.0;
	}

	bool hit_world(vec3 rayOrigin, vec3 rayDir, out HitRecord rec) {
		float minT = 1e9;
		int hitIdx = -1;
		for (int i = 0; i < sphereCount; i++) {
			vec3 sphereCenter = spheres[i*2].xyz;
			float sphereRadius = spheres[i*2].w;
			float t = hit_sphere(sphereCenter, sphereRadius, rayOrigin, rayDir);
			if (t > 0.0 && t < minT) {
				minT = t;
				hitIdx = i;
			}
		}
		if (hitIdx >= 0) {
			vec3 sphereCenter = spheres[hitIdx*2].xyz;
			float sphereRadius = spheres[hitIdx*2].w;
			vec3 sphereColor = spheres[hitIdx*2+1].xyz;
			rec.t = minT;
			rec.idx = hitIdx;
			rec.pos = rayOrigin + minT * rayDir;
			rec.normal = normalize(rec.pos - sphereCenter);
			rec.color = sphereColor;
			return true;
		}
		return false;
	}

	vec3 set_sphere_color(vec3 sphereColor, vec3 normal, vec3 lightDir, vec3 viewDir) {
		float diffuse = max(dot(normal, lightDir), 0.0);
		vec3 reflectDir = reflect(-lightDir, normal);
		float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
		return sphereColor * diffuse + vec3(1.0) * spec * 0.3;
	}

	void main() {
		vec3 color = vec3(0.0);
		for (int s = 0; s < int(samples_per_pixel); s++) {
			// 隨機 jitter
			vec2 jitter = random_vec2(float(s) + vec2(0));
			vec2 uv = get_uv(gl_FragCoord.xy, jitter, u_resolution);

			// 相機位置與朝向由 uniform 控制
			vec3 rayOrigin = cameraPos;
			vec3 forward = normalize(cameraTarget - cameraPos);
			vec3 up = vec3(0.0, 1.0, 0.0);
			vec3 right = normalize(cross(forward, up));
			up = cross(right, forward);
			vec3 rayDir = normalize(forward + uv.x * right * cameraFov + uv.y * up * cameraFov);

			vec3 attenuation = vec3(1.0);
			for (int depth = 0; depth < max_depth; depth++) {
				HitRecord rec;
				if (hit_world(rayOrigin, rayDir, rec)) {
					rayOrigin = rec.pos + rec.normal * EPSILON;
					rayDir = normalize(random_in_hemisphere(rec.normal, rec.pos.xy + float(depth) + float(s)));
					attenuation *= 0.5 * rec.color;
				} else {
					// 天空漸層
					vec3 unit_direction = normalize(rayDir);
					float a = 0.5 * (unit_direction.y + 1.0);
					//vec3 sky = mix(vec3(1.0, 1.0, 1.0), backgroundColor, a);
					vec3 sky = vec3(0.01, 0.01, 0.01);
					color += attenuation * sky;
					break;
				}
			}
		}
		color /= samples_per_pixel;
		FragColor = vec4(color, 1.0);
	}
