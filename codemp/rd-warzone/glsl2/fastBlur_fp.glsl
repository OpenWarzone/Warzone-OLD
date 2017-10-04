uniform sampler2D		u_DiffuseMap;

uniform vec4			u_ViewInfo; // zmin, zmax, zmax / zmin
uniform float			u_ShadowZfar[5];
uniform vec2			u_Dimensions;
uniform vec4			u_Settings0;

varying vec2			var_TexCoords;

#define BLUR_RADIUS		u_Settings0.r
#define BLUR_PIXELMULT	u_Settings0.g

#define px (1.0/u_Dimensions.x)*BLUR_PIXELMULT
#define py (1.0/u_Dimensions.y)*BLUR_PIXELMULT

#define RADIUS_X (BLUR_RADIUS * px)
#define RADIUS_Y (BLUR_RADIUS * py)

float DistFromDepth(float depth)
{
	return depth * u_ViewInfo.y;//(u_ViewInfo.y / u_ViewInfo.x);
}

vec4 FastBlur(void)
{
	vec2 shadowInfo = texture(u_DiffuseMap, var_TexCoords.xy).xy;
	float color = shadowInfo.x;
	float depth = shadowInfo.y;
	float dist = DistFromDepth(depth);
	float invDepth = 1.0 + (1.0 - depth);
	invDepth *= 0.666;

	int NUM_BLUR_PIXELS = 1;

//#pragma unroll ((BLUR_RADIUS * 2.0) / BLUR_PIXELMULT)
	for (float x = -RADIUS_X * invDepth; x <= RADIUS_X * invDepth; x += px * invDepth)
	{
		for (float y = -RADIUS_Y * invDepth; y <= RADIUS_Y * invDepth; y += py * invDepth)
		{
			vec2 thisInfo = texture(u_DiffuseMap, vec2(var_TexCoords.x + x, var_TexCoords.y + y)).xy;

			float thisDist = DistFromDepth(thisInfo.y);
			
			if (length(thisDist - dist) > 16.0/*8.0*/) continue;

			color += thisInfo.r;
			NUM_BLUR_PIXELS++;
		}
	}

	color /= NUM_BLUR_PIXELS;
	return vec4(color, 0.0, 0.0, 1.0);
}

void main()
{
	gl_FragColor = FastBlur();
}
