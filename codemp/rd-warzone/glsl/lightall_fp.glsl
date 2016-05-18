#define						USE_TRI_PLANAR
//#define					USE_SUBSURFACE_SCATTER
//#define					EXPERIMENTAL_LIGHT

uniform sampler2D			u_DiffuseMap;
uniform sampler2D			u_SteepMap;
uniform sampler2D			u_SteepMap2;
uniform sampler2D			u_SplatControlMap;
uniform sampler2D			u_SplatMap1;
uniform sampler2D			u_SplatMap2;
uniform sampler2D			u_SplatMap3;
uniform sampler2D			u_SplatMap4;
uniform sampler2D			u_SplatNormalMap1;
uniform sampler2D			u_SplatNormalMap2;
uniform sampler2D			u_SplatNormalMap3;
uniform sampler2D			u_SplatNormalMap4;

uniform sampler2D			u_LightMap;

uniform sampler2D			u_NormalMap;
uniform sampler2D			u_NormalMap2;
uniform sampler2D			u_NormalMap3;

uniform sampler2D			u_DeluxeMap;

#if defined(USE_SPECULARMAP)
uniform sampler2D			u_SpecularMap;
#endif

#if defined(USE_SHADOWMAP)
uniform sampler2D			u_ShadowMap;
#endif

#if defined(USE_CUBEMAP)
#define textureCubeLod textureLod // UQ1: > ver 140 support
uniform samplerCube			u_CubeMap;
#endif

#if defined(USE_SUBSURFACE_SCATTER)
uniform sampler2D			u_SubsurfaceMap;
#endif

uniform sampler2D			u_OverlayMap;



uniform vec2				u_Dimensions;
uniform vec4				u_Local1; // parallaxScale, haveSpecular, specularScale, materialType
uniform vec4				u_Local2; // ExtinctionCoefficient
uniform vec4				u_Local3; // RimScalar, MaterialThickness, subSpecPower, cubemapScale
uniform vec4				u_Local4; // haveNormalMap, isMetalic, hasRealSubsurfaceMap, sway
uniform vec4				u_Local5; // hasRealOverlayMap, overlaySway, blinnPhong, hasSteepMap
uniform vec4				u_Local6; // useSunLightSpecular, hasSteepMap2, MAP_SIZE, WATER_LEVEL
uniform vec4				u_Local7; // hasSplatMap1, hasSplatMap2, hasSplatMap3, hasSplatMap4
uniform vec4				u_Local9; // testvalue0, 1, 2, 3

#define WATER_LEVEL			u_Local6.a

uniform mat4				u_ModelViewProjectionMatrix;
uniform mat4				u_ModelMatrix;
uniform mat4				u_invEyeProjectionMatrix;
uniform mat4				u_ModelViewMatrix;

uniform vec4				u_EnableTextures;

uniform vec4				u_PrimaryLightOrigin;
uniform vec3				u_PrimaryLightColor;
uniform vec3				u_PrimaryLightAmbient;

uniform vec4				u_NormalScale;
uniform vec4				u_SpecularScale;

uniform vec4				u_CubeMapInfo;
uniform float				u_CubeMapStrength;


uniform int					u_lightCount;
uniform vec3				u_lightPositions2[16];
uniform float				u_lightDistances[16];
uniform vec3				u_lightColors[16];

uniform vec3				u_ViewOrigin;


#if defined(USE_TESSELLATION)

in vec3						Normal_FS_in;
in vec2						TexCoord_FS_in;
in vec3						WorldPos_FS_in;
in vec3						ViewDir_FS_in;
in vec4						Tangent_FS_in;
in vec4						Bitangent_FS_in;

in vec4						Color_FS_in;
in vec4						PrimaryLightDir_FS_in;
in vec2						TexCoord2_FS_in;

in vec3						Blending_FS_in;
in float					Slope_FS_in;
in float					usingSteepMap_FS_in;


vec3 m_Normal =				normalize(-Normal_FS_in.xyz);

#define m_TexCoords			TexCoord_FS_in
#define m_vertPos			WorldPos_FS_in
#define m_ViewDir			ViewDir_FS_in

#define var_Normal2			ViewDir_FS_in.x

#define var_Tangent			Tangent_FS_in
#define var_Bitangent		Bitangent_FS_in

#define var_nonTCtexCoords	TexCoord_FS_in
#define var_Color			Color_FS_in
#define	var_PrimaryLightDir PrimaryLightDir_FS_in
#define var_TexCoords2		TexCoord2_FS_in

#define var_Blending		Blending_FS_in
#define var_Slope			Slope_FS_in
#define var_usingSteepMap	usingSteepMap_FS_in


#else //!defined(USE_TESSELLATION)

varying vec2				var_TexCoords;
varying vec2				var_TexCoords2;
varying vec4				var_Normal;

#define var_Normal2			var_Normal.w

varying vec4				var_Tangent;
varying vec4				var_Bitangent;
varying vec4				var_Color;

varying vec4				var_PrimaryLightDir;

varying vec3				var_vertPos;

varying vec3				var_ViewDir;
varying vec2				var_nonTCtexCoords; // for steep maps


varying vec3				var_Blending;
varying float				var_Slope;
varying float				var_usingSteepMap;


#define m_Normal			var_Normal
#define m_TexCoords			var_TexCoords
#define m_vertPos			var_vertPos
#define m_ViewDir			var_ViewDir


#endif //defined(USE_TESSELLATION)



out vec4 out_Glow;
out vec4 out_Position;
out vec4 out_Normal;


// For fake normal map lookups.
#define FAKE_MAP_NONE 0
#define FAKE_MAP_NORMALMAP 1
#define FAKE_MAP_NORMALMAP2 2
#define FAKE_MAP_NORMALMAP3 3
#define FAKE_MAP_SPLATNORMALMAP1 4
#define FAKE_MAP_SPLATNORMALMAP2 5
#define FAKE_MAP_SPLATNORMALMAP3 6
#define FAKE_MAP_SPLATNORMALMAP4 7


vec3 vLocalSeed;

// This function returns random number from zero to one
float randZeroOne()
{
    uint n = floatBitsToUint(vLocalSeed.y * 214013.0 + vLocalSeed.x * 2531011.0 + vLocalSeed.z * 141251.0);
    n = n * (n * n * 15731u + 789221u);
    n = (n >> 9u) | 0x3F800000u;
 
    float fRes =  2.0 - uintBitsToFloat(n);
    vLocalSeed = vec3(vLocalSeed.x + 147158.0 * fRes, vLocalSeed.y*fRes  + 415161.0 * fRes, vLocalSeed.z + 324154.0*fRes);
    return fRes;
}

vec3 splatblend(vec4 texture1, float a1, vec4 texture2, float a2)
{
    float depth = 0.2;
    float ma = max(texture1.a + a1, texture2.a + a2) - depth;

    float b1 = max(texture1.a + a1 - ma, 0);
    float b2 = max(texture2.a + a2 - ma, 0);

    return (texture1.rgb * b1 + texture2.rgb * b2) / (b1 + b2);
}

vec4 GetControlMap( sampler2D tex, float scale)
{
	vec4 xaxis = texture2D( tex, (m_vertPos.yz * scale) * 0.5 + 0.5);
	vec4 yaxis = texture2D( tex, (m_vertPos.xz * scale) * 0.5 + 0.5);
	vec4 zaxis = texture2D( tex, (m_vertPos.xy * scale) * 0.5 + 0.5);

	return xaxis * var_Blending.x + yaxis * var_Blending.y + zaxis * var_Blending.z;
}

vec4 ConvertToNormals ( vec4 color )
{
	// This makes silly assumptions, but it adds variation to the output. Hopefully this will look ok without doing a crapload of texture lookups or
	// wasting vram on real normals.
	//
	// UPDATE: In my testing, this method looks just as good as real normal maps. I am now using this as default method unless r_normalmapping >= 2
	// for the very noticable FPS boost over texture lookups.

	//N = vec3((color.r + color.b) / 2.0, (color.g + color.b) / 2.0, (color.r + color.g) / 2.0);
	vec3 N = vec3(clamp(color.r + color.b, 0.0, 1.0), clamp(color.g + color.b, 0.0, 1.0), clamp(color.r + color.g, 0.0, 1.0));
	N.xy = 1.0 - N.xy;
	vec4 norm = vec4(N, 1.0 - (length(N.xyz) / 3.0));
	return norm;
}

vec4 GetMap( in sampler2D tex, float scale, vec2 ParallaxOffset, int fakeMapType)
{
	bool fakeNormal = false;

	if (u_Local4.r <= 0.0 && fakeMapType == FAKE_MAP_NORMALMAP)
	{
		tex = u_DiffuseMap;
		fakeNormal = true;
	}
	else if (u_Local4.r <= 0.0 && fakeMapType == FAKE_MAP_NORMALMAP2)
	{
		tex = u_SteepMap;
		fakeNormal = true;
	}
	else if (u_Local4.r <= 0.0 && fakeMapType == FAKE_MAP_NORMALMAP3)
	{
		tex = u_SteepMap2;
		fakeNormal = true;
	}
	else if (u_Local4.r <= 0.0 && fakeMapType == FAKE_MAP_SPLATNORMALMAP1)
	{
		tex = u_SplatMap1;
		fakeNormal = true;
	}
	else if (u_Local4.r <= 0.0 && fakeMapType == FAKE_MAP_SPLATNORMALMAP2)
	{
		tex = u_SplatMap2;
		fakeNormal = true;
	}
	else if (u_Local4.r <= 0.0 && fakeMapType == FAKE_MAP_SPLATNORMALMAP3)
	{
		tex = u_SplatMap3;
		fakeNormal = true;
	}
	else if (u_Local4.r <= 0.0 && fakeMapType == FAKE_MAP_SPLATNORMALMAP4)
	{
		tex = u_SplatMap4;
		fakeNormal = true;
	}

	vec4 xaxis = texture2D( tex, (m_vertPos.yz * scale) + ParallaxOffset.xy);
	vec4 yaxis = texture2D( tex, (m_vertPos.xz * scale) + ParallaxOffset.xy);
	vec4 zaxis = texture2D( tex, (m_vertPos.xy * scale) + ParallaxOffset.xy);

	if (fakeNormal)
	{
		return ConvertToNormals(xaxis * var_Blending.x + yaxis * var_Blending.y + zaxis * var_Blending.z);
	}

	return xaxis * var_Blending.x + yaxis * var_Blending.y + zaxis * var_Blending.z;
}

vec4 GetNonSplatMap( in sampler2D tex, vec2 coord, int fakeMapType )
{
	bool fakeNormal = false;

	if (u_Local4.r <= 0.0 && fakeMapType == FAKE_MAP_NORMALMAP)
	{
		tex = u_DiffuseMap;
		fakeNormal = true;
	}
	else if (u_Local4.r <= 0.0 && fakeMapType == FAKE_MAP_NORMALMAP2)
	{
		tex = u_SteepMap;
		fakeNormal = true;
	}
	else if (u_Local4.r <= 0.0 && fakeMapType == FAKE_MAP_NORMALMAP3)
	{
		tex = u_SteepMap2;
		fakeNormal = true;
	}
	else if (u_Local4.r <= 0.0 && fakeMapType == FAKE_MAP_SPLATNORMALMAP1)
	{
		tex = u_SplatMap1;
		fakeNormal = true;
	}
	else if (u_Local4.r <= 0.0 && fakeMapType == FAKE_MAP_SPLATNORMALMAP2)
	{
		tex = u_SplatMap2;
		fakeNormal = true;
	}
	else if (u_Local4.r <= 0.0 && fakeMapType == FAKE_MAP_SPLATNORMALMAP3)
	{
		tex = u_SplatMap3;
		fakeNormal = true;
	}
	else if (u_Local4.r <= 0.0 && fakeMapType == FAKE_MAP_SPLATNORMALMAP4)
	{
		tex = u_SplatMap4;
		fakeNormal = true;
	}

	vec4 map = texture2D( tex, coord );

	if (fakeNormal)
	{
		return ConvertToNormals(map);
	}

	return map;
}

vec4 GetSplatMap(vec2 texCoords, vec2 ParallaxOffset, float pixRandom, vec4 inColor, float inA1)
{
#if defined(USE_OVERLAY) || defined(USE_TRI_PLANAR)
	if (u_Local7.r <= 0.0 && u_Local7.g <= 0.0 && u_Local7.b <= 0.0 && u_Local7.a <= 0.0)
	{
		return inColor;
	}

	if (u_Local6.g > 0.0 && m_vertPos.z <= WATER_LEVEL + 128.0 + (64.0 * pixRandom))
	{// Steep maps (water edges)... Underwater and beach doesn't use splats for now...
		return inColor;
	}

	float origInA1 = inA1;
	const float scale = 0.01;
	float controlScale = 1.0 / u_Local6.b;

	// Splat blend in all the textues using the control strengths...

	vec4 splatColor = inColor;

	vec4 control = GetControlMap(u_SplatControlMap, controlScale);
	control = clamp(pow(control, vec4(2.5)) * 10.0, 0.0, 1.0);

	if (length(control.rgba) <= 0.0)
	{
		return inColor;
	}

	bool foundSplat = false;

	if (u_Local7.r > 0.0 && control.r > 0.0)
		foundSplat = true;
	
	if (!foundSplat && u_Local7.g > 0.0 && control.g > 0.0)
		foundSplat = true;

	if (!foundSplat && u_Local7.b > 0.0 && control.b > 0.0)
		foundSplat = true;

	if (!foundSplat && u_Local7.a > 0.0 && control.a > 0.0)
		foundSplat = true;

	if (!foundSplat)
	{// There is nothing valid to do here... Skip blending...
		return inColor;
	}

	if (u_Local7.r > 0.0)
	{
		vec4 tex = GetMap(u_SplatMap1, scale, ParallaxOffset, FAKE_MAP_NONE);
		splatColor = mix(splatColor, tex, control.r * tex.a);
		//float a1 = smoothstep(0.0, 1.0, GetMap(u_SplatNormalMap1, scale, ParallaxOffset).a * control.r * tex.a * u_Local9.a);
		//splatColor = vec4(splatblend(splatColor, inA1, tex, a1), 1.0);
	}

	if (u_Local7.g > 0.0)
	{
		vec4 tex = GetMap(u_SplatMap2, scale, ParallaxOffset, FAKE_MAP_NONE);
		splatColor = mix(splatColor, tex, control.g * tex.a);
		//float a1 = smoothstep(0.0, 1.0, GetMap(u_SplatNormalMap2, scale, ParallaxOffset).a * control.g * tex.a * u_Local9.a);
		//splatColor = vec4(splatblend(splatColor, inA1, tex, a1), 1.0);
	}

	if (u_Local7.b > 0.0)
	{
		vec4 tex = GetMap(u_SplatMap3, scale, ParallaxOffset, FAKE_MAP_NONE);
		splatColor = mix(splatColor, tex, control.b * tex.a);
		//float a1 = smoothstep(0.0, 1.0, GetMap(u_SplatNormalMap3, scale, ParallaxOffset).a * control.b * tex.a * u_Local9.a);
		//splatColor = vec4(splatblend(splatColor, inA1, tex, a1), 1.0);
	}
	/*
	if (u_Local7.a > 0.0)
	{
		vec4 tex = GetMap(u_SplatMap4, scale, ParallaxOffset, FAKE_MAP_NONE);
		splatColor = mix(splatColor, tex, control.a * tex.a);
		//float a1 = smoothstep(0.0, 1.0, GetMap(u_SplatNormalMap4, scale, ParallaxOffset).a * control.a * tex.a);
		//splatColor = vec4(splatblend(splatColor, inA1, tex, a1), 1.0);
	}
	*/

	///splatColor = vec4(splatblend(inColor, origInA1/* * u_Local9.a*/, splatColor, 1.0 - (origInA1/* * u_Local9.a*/)), 1.0);
	//splatColor = mix(inColor, splatColor, (length(control.rgba) / 4.0)/*splatColor.a*/);
	return splatColor;
#else //!defined(USE_OVERLAY) || defined(USE_TRI_PLANAR)
	return inColor;
#endif //defined(USE_OVERLAY) || defined(USE_TRI_PLANAR)
}

vec4 GetDiffuse(vec2 texCoords, vec2 ParallaxOffset, float pixRandom)
{
#if defined(USE_OVERLAY) || defined(USE_TRI_PLANAR)
	if (u_Local6.g > 0.0 && m_vertPos.z <= WATER_LEVEL + 128.0 + (64.0 * pixRandom))
	{// Steep maps (water edges)...
		float mixVal = ((WATER_LEVEL + 128.0) - m_vertPos.z) / 128.0;
		const float scale = 0.01;

		vec4 tex1 = GetMap(u_SteepMap2, scale, ParallaxOffset, FAKE_MAP_NONE);
		float a1 = 0.0;
		
		if (u_Local4.r <= 0.0) // save texture lookup
			a1 = ConvertToNormals(tex1).a;
		else
			a1 = GetMap(u_NormalMap3, scale, ParallaxOffset, FAKE_MAP_NORMALMAP3).a;

		vec4 tex2 = GetMap(u_DiffuseMap, scale, ParallaxOffset, FAKE_MAP_NONE);
		float a2 = 0.0;

		if (u_Local4.r <= 0.0) // save texture lookup
			a2 = ConvertToNormals(tex2).a;
		else
			a2 = GetMap(u_NormalMap, scale, ParallaxOffset, FAKE_MAP_NORMALMAP).a;

		if (u_Local7.r <= 0.0 && u_Local7.g <= 0.0 && u_Local7.b <= 0.0 && u_Local7.a <= 0.0)
		{// No splat maps...
			return vec4(splatblend(tex1, a1 * (a1 * mixVal), tex2, a2 * (1.0 - (a2 * mixVal))), 1.0);
		}

		// Splat mapping...
		tex2 = GetSplatMap(texCoords, ParallaxOffset, pixRandom, tex2, a2);

		return vec4(splatblend(tex1, a1 * (a1 * mixVal), tex2, a2 * (1.0 - (a2 * mixVal))), 1.0);
	}
	else if (u_Local5.a > 0.0 && var_Slope > 0)
	{// Steep maps (high angles)...
		const float scale = 0.0025;

		if (u_Local7.r <= 0.0 && u_Local7.g <= 0.0 && u_Local7.b <= 0.0 && u_Local7.a <= 0.0)
		{// No splat maps...
			return GetMap(u_SteepMap, scale, ParallaxOffset, FAKE_MAP_NONE);
		}

		vec4 tex = GetMap(u_SteepMap, scale, ParallaxOffset, FAKE_MAP_NONE);
		float a1 = 0.0;
		
		if (u_Local4.r <= 0.0) // save texture lookup
			a1 = ConvertToNormals(tex).a;
		else
			GetMap(u_NormalMap, scale, ParallaxOffset, FAKE_MAP_NORMALMAP2).a;//GetMap(u_NormalMap2, scale, ParallaxOffset).a;

		return GetSplatMap(texCoords, ParallaxOffset, pixRandom, tex, a1);
	}
	else if (u_Local5.a > 0.0)
	{// Steep maps (low angles)...
		const float scale = 0.01;

		if (u_Local7.r <= 0.0 && u_Local7.g <= 0.0 && u_Local7.b <= 0.0 && u_Local7.a <= 0.0)
		{// No splat maps...
			return GetMap(u_DiffuseMap, scale, ParallaxOffset, FAKE_MAP_NONE);
		}

		// Splat mapping...
		vec4 tex = GetMap(u_DiffuseMap, scale, ParallaxOffset, FAKE_MAP_NONE);
		float a1 = 0.0;

		if (u_Local4.r <= 0.0) // save texture lookup
			a1 = ConvertToNormals(tex).a;
		else
			a1 = GetMap(u_NormalMap, scale, ParallaxOffset, FAKE_MAP_NORMALMAP).a;

		return GetSplatMap(texCoords, ParallaxOffset, pixRandom, tex, a1);
	}
	else
	{
		return GetNonSplatMap(u_DiffuseMap, texCoords, FAKE_MAP_NONE);
	}
#else //!defined(USE_OVERLAY) || defined(USE_TRI_PLANAR)
	return GetNonSplatMap(u_DiffuseMap, texCoords, FAKE_MAP_NONE);
#endif //defined(USE_OVERLAY) || defined(USE_TRI_PLANAR)
}

vec4 GetNormal(vec2 texCoords, vec2 ParallaxOffset, float pixRandom)
{
#if defined(USE_OVERLAY) || defined(USE_TRI_PLANAR)
	if (u_Local6.g > 0.0 && m_vertPos.z <= WATER_LEVEL + 128.0 + (64.0 * pixRandom))
	{// Steep maps (water edges)...
		float mixVal = ((WATER_LEVEL + 128.0) - m_vertPos.z) / 128.0;
		const float scale = 0.01;

		vec4 tex1 = GetMap(u_NormalMap3, scale, ParallaxOffset, FAKE_MAP_NORMALMAP3);
		float a1 = tex1.a;

		vec4 tex2 = GetMap(u_NormalMap, scale, ParallaxOffset, FAKE_MAP_NORMALMAP);
		float a2 = tex2.a;
		
		return vec4(splatblend(tex1, a1 * (a1 * mixVal), tex2, a2 * (1.0 - (a2 * mixVal))), 1.0);
	}
	else if (u_Local5.a > 0.0 && var_Slope > 0)
	{// Steep maps (high angles)...
		const float scale = 0.0025;
		return GetMap(u_NormalMap2, scale, ParallaxOffset, FAKE_MAP_NORMALMAP2);//GetMap(u_NormalMap2, scale, ParallaxOffset);
	}
	else if (u_Local5.a > 0.0)
	{// Steep maps (normal angles)...
		const float scale = 0.01;
		return GetMap(u_NormalMap, scale, ParallaxOffset, FAKE_MAP_NORMALMAP);
	}
	else
	{
		return GetNonSplatMap(u_NormalMap, texCoords, FAKE_MAP_NORMALMAP);
	}
#else //!defined(USE_OVERLAY) || defined(USE_TRI_PLANAR)
	return GetNonSplatMap(u_NormalMap, texCoords, FAKE_MAP_NORMALMAP);
#endif //defined(USE_OVERLAY) || defined(USE_TRI_PLANAR)
}

float GetDepth(vec2 t)
{
	return 1.0 - GetNormal(t, vec2(0.0), 0.0).a;
}

#if defined(USE_PARALLAXMAP)

float RayIntersectDisplaceMap(vec2 dp)
{
	if (u_Local1.x == 0.0)
		return 0.0;
	
	float depth = GetDepth(dp) - 1.0;
	return depth * u_Local1.x;
}
#endif

vec3 EnvironmentBRDF(float gloss, float NE, vec3 specular)
{
	vec4 t = vec4( 1/0.96, 0.475, (0.0275 - 0.25 * 0.04)/0.96,0.25 ) * gloss;
	t += vec4( 0.0, 0.0, (0.015 - 0.75 * 0.04)/0.96,0.75 );
	float a0 = t.x * min( t.y, exp2( -9.28 * NE ) ) + t.z;
	float a1 = t.w;
	return clamp( a0 + specular * ( a1 - a0 ), 0.0, 1.0 );
}



float blinnPhongSpecular(in vec3 normalVec, in vec3 lightVec, in float specPower)
{
    vec3 halfAngle = normalize(normalVec + lightVec);
    return pow(clamp(dot(normalVec,halfAngle),0.0,1.0),specPower);
}

 
#ifdef USE_SUBSURFACE_SCATTER
float halfLambert(vec3 vect1, vec3 vect2)
{
	float product = dot(vect1,vect2);
	return product * 0.5 + 0.5;
}

// Main fake sub-surface scatter lighting function

vec3 ExtinctionCoefficient = u_Local2.xyz;
float RimScalar = u_Local3.x;
float MaterialThickness = u_Local3.y;
float SpecPower = u_Local3.z;

vec4 subScatterFS(vec4 BaseColor, vec4 SpecColor, vec3 lightVec, vec3 LightColor, vec3 eyeVec, vec3 worldNormal, vec2 texCoords)
{
	vec4 subsurface = vec4(ExtinctionCoefficient, MaterialThickness);

	if (u_Local4.z != 0.0)
	{// We have a subsurface image, use it instead...
		subsurface = texture2D(u_SubsurfaceMap, texCoords.xy);
	}
	else if (length(ExtinctionCoefficient) == 0.0)
	{// Default if not specified...
		subsurface.rgb = vec3(BaseColor.rgb);
	}

	if (MaterialThickness == 0.0)
	{// Default if not specified...
		MaterialThickness = 0.01;
	}
	
	if (subsurface.a == 0.0 && MaterialThickness != 0.0)
	{// Backup in case image is missing alpha channel...
		subsurface.a = MaterialThickness;
	}

	subsurface.a = 1.0 - subsurface.a;

	if (RimScalar == 0.0)
	{// Default if not specified...
		RimScalar = 0.5;
	}

	if (SpecPower == 0.0)
	{// Default if not specified...
		SpecPower = 0.3;
	}

	float attenuation = 1.0;//10.0 * (1.0 / distance(lightVec.xyz,m_vertPos.xyz));
    vec3 eVec = eyeVec;
    vec3 lVec = lightVec;
    vec3 wNorm = worldNormal;
     
    //vec4 dotLN = vec4(halfLambert(lVec,wNorm) * attenuation);
	vec3 halfDir2 = normalize(lVec + eVec);
	float specAngle = max(dot(halfDir2, wNorm), 0.0);
	vec4 dotLN = vec4(specAngle * attenuation);

    dotLN *= BaseColor;


	vec3 halfDir3 = normalize(lVec + -eVec);
	float specAngle2 = max(dot(halfDir3, -wNorm), 0.0);
     
    vec3 indirectLightComponent = vec3(subsurface.a * max(vec3(0.0),length(halfDir3)/3.0/*dot(-wNorm,lVec)*/));
    indirectLightComponent += subsurface.a * specAngle2;//halfLambert(-eVec,lVec);
    indirectLightComponent *= attenuation;
    indirectLightComponent.rgb *= subsurface.rgb;
     
    vec3 rim = vec3(1.0 - max(0.0,dot(wNorm,eVec)));
    rim *= rim;
    rim *= max(0.0,dot(wNorm,lVec)) * SpecColor.rgb;
     
    vec4 finalCol = dotLN + vec4(indirectLightComponent,1.0);
    finalCol.rgb += (rim * RimScalar * attenuation * finalCol.a);
    //finalCol.rgb += vec3(blinnPhongSpecular(wNorm,lVec,SpecPower) * attenuation * SpecColor * finalCol.a * 0.05);
    finalCol.rgb *= LightColor.rgb;
     
    return finalCol;   
}
#endif //USE_SUBSURFACE_SCATTER

#if defined(EXPERIMENTAL_LIGHT)
vec3 ExperimentalLighting(vec3 vSurfacePos, vec3 vViewPos, vec3 vLightPos, vec3 normal, vec3 lightColor, float strength, vec3 diffuse)
{
	vec3 vDirToView = normalize( vViewPos - vSurfacePos );
	vec3 vDirToLight = normalize( vLightPos - vSurfacePos );

	float fNDotL = clamp( dot(normal, vDirToLight), 0.0, 1.0);
	float fDiffuse = fNDotL;
	
	vec3 vHalf = normalize( vDirToView + vDirToLight );
	float fNDotH = clamp( dot(normal, vHalf), 0.0, 1.0);
	float fSpec = pow(fNDotH, 10.0) * fNDotL * 0.5;
	
	vec3 vResult = ((lightColor.rgb * fDiffuse * strength) + (lightColor.rgb * strength * fSpec)) * diffuse;
	
	vResult = sqrt(vResult);
	return vResult;
}
#endif //defined(EXPERIMENTAL_LIGHT)

//---------------------------------------------------------------------------------------------
// Normal Blending Techniques
//---------------------------------------------------------------------------------------------

// RNM
vec3 NormalBlend_RNM(vec3 n1, vec3 n2)
{
    // Unpack (see article on why it's not just n*2-1)
	n1 = n1*vec3( 2,  2, 2) + vec3(-1, -1,  0);
    n2 = n2*vec3(-2, -2, 2) + vec3( 1,  1, -1);
    
    // Blend
    return n1*dot(n1, n2)/n1.z - n2;
}

// Linear Blending
vec3 NormalBlend_Linear(vec3 n1, vec3 n2)
{
    // Unpack
	n1 = n1*2.0 - 1.0;
    n2 = n2*2.0 - 1.0;
    
	return normalize(n1 + n2);    
}

#define TECHNIQUE_RNM 				 0
#define TECHNIQUE_Linear		     1

// Combine normals
vec3 CombineNormal(vec3 n1, vec3 n2, int technique)
{
 	if (technique == TECHNIQUE_RNM)
        return NormalBlend_RNM(n1, n2);
    else
        return NormalBlend_Linear(n1, n2);
}

void main()
{
	vec3 viewDir = vec3(0.0), lightColor = vec3(0.0), ambientColor = vec3(0.0);
	vec4 specular = vec4(0.0);
	vec3 N, E, H;
	vec3 DETAILED_NORMAL = vec3(1.0);
	float NL, NH, NE, EH, attenuation;
	vec2 tex_offset = vec2(1.0 / u_Dimensions);
	vec2 texCoords = m_TexCoords.xy;

	vLocalSeed = m_vertPos.xyz;

	float pixRandom = randZeroOne();// * 0.5 + 0.5;
	//float pixRandom = (clamp(m_TexCoords.x, 0.0, 1.0) + clamp(m_TexCoords.y, 0.0, 1.0)) / 2.0;
	//if (pixRandom < 0.5) pixRandom = 0.5 + pixRandom;

	
	#if 0
	#if defined(USE_TESSELLATION)
	//if (u_Local6.g > 0.0 && m_vertPos.z <= WATER_LEVEL + 32.0)
	{
		//gl_FragColor = vec4(m_Normal.xyz * 0.5 + 0.5, 1.0);
		gl_FragColor = vec4(vec3(texture2D( u_NormalMap, texCoords).a), 1.0);

		#if defined(USE_GLOW_BUFFER)
			out_Glow = gl_FragColor;
		#else
			out_Glow = vec4(0.0);
		#endif

		out_Normal = vec4(m_Normal.xyz * 0.5 + 0.5, 0.2);
		out_Position = vec4(m_vertPos, u_Local1.a / MATERIAL_LAST);
		return;
	}
	#endif //defined(USE_TESSELLATION)
	#endif


	#ifdef USE_OVERLAY
		if (u_Local4.a > 0.0)
		{// Sway...
			texCoords += vec2(u_Local5.y * u_Local4.a * ((1.0 - m_TexCoords.y) + 1.0), 0.0);
		}
	#endif //USE_OVERLAY




	mat3 tangentToWorld = mat3(var_Tangent.xyz, var_Bitangent.xyz, m_Normal.xyz);
	

	#if !defined(USE_TESSELLATION)
		viewDir = vec3(var_Normal2, var_Tangent.w, var_Bitangent.w);
	#else //defined(USE_TESSELLATION)
		viewDir = m_ViewDir.xyz;
	#endif //defined(USE_TESSELLATION)

	E = normalize(viewDir);



	#if defined(USE_LIGHTMAP)

		vec4 lightmapColor = texture2D(u_LightMap, var_TexCoords2.st);
  
		#if defined(RGBM_LIGHTMAP)
			lightmapColor.rgb *= lightmapColor.a;
		#endif //defined(RGBM_LIGHTMAP)
 
	#endif //defined(USE_LIGHTMAP)



	vec2 ParallaxOffset = vec2(0.0);


	#if defined(USE_PARALLAXMAP)
		vec3 offsetDir = normalize(E * tangentToWorld);
		vec2 ParallaxXY = vec2(0.0);

		if (u_Local5.a > 0.0 && var_Slope > 0)
		{// Steep maps...
			ParallaxXY = offsetDir.xy * 2.5; // Always rock?!?!?!?
		}
		else
		{
			ParallaxXY = offsetDir.xy * u_Local1.x;
		}

		#if defined(FAST_PARALLAX)

			ParallaxOffset = ParallaxXY * RayIntersectDisplaceMap(texCoords);
			texCoords += ParallaxOffset;

		#else //!defined(FAST_PARALLAX)

			// Steep Parallax
			float Step = 0.01;
			vec2 dt = ParallaxXY * Step;
			float Height = 0.5;
			float oldHeight = 0.5;
			vec2 Coord = texCoords;
			vec2 oldCoord = Coord;
			float HeightMap = GetDepth( Coord );
			float oldHeightMap = HeightMap;
 
			while( HeightMap < Height )
			{
				oldHeightMap = HeightMap;
				oldHeight = Height;
				oldCoord = Coord;
 
				Height -= Step;
				Coord += dt;
				HeightMap = GetDepth( Coord );
			}
		
			Coord = (Coord + oldCoord)*0.5;
			if( Height < 0.0 )
			{
				Coord = oldCoord;
				Height = 0.0;
			}
		
			ParallaxOffset = Coord - texCoords;
			texCoords = Coord;

		#endif //defined(FAST_PARALLAX)

	#endif //defined(USE_PARALLAXMAP)




	vec4 diffuse = GetDiffuse(texCoords, ParallaxOffset, pixRandom);

	#if defined(USE_GAMMA2_TEXTURES)
		diffuse.rgb *= diffuse.rgb;
	#endif


	vec4 norm = vec4(m_Normal.xyz, 0.0);

	norm = GetNormal(texCoords, ParallaxOffset, pixRandom);
	N = norm.xyz * 2.0 - 1.0;
	//N = CombineNormal(m_Normal.xyz * 0.5 + 0.5, norm.xyz, int(u_Local9.r));
	N.xy *= u_NormalScale.xy;
	N.z = sqrt(clamp((0.25 - N.x * N.x) - N.y * N.y, 0.0, 1.0));
	N = tangentToWorld * N;
	N = normalize(N);

	DETAILED_NORMAL = N;

	

	#if defined(USE_OVERLAY)

		//
		// Overlay Maps...
		//

		#define OVERLAY_HEIGHT 40.0

		if (u_Local5.x > 0.0)
		{// Have overlay map...
			vec2 ovCoords = m_TexCoords.xy + vec2(u_Local5.y); // u_Local5.y == sway ammount
			vec4 overlay = texture2D(u_OverlayMap, ovCoords);

			if (overlay.a > 0.1)
			{// Have an overlay, and it is visible here... Set it as diffuse instead...
				diffuse = overlay;
			}
			else
			{// Have an overlay, but it is not visibile at this pixel... Still need to check if we need a shadow casted on this pixel...
				vec2 ovCoords2 = ovCoords - (tex_offset * OVERLAY_HEIGHT);
				vec4 overlay2 = texture2D(u_OverlayMap, ovCoords2);

				if (overlay2.a > 0.1)
				{// Add shadow...
					diffuse.rgb *= 0.25;
				}
			}
		}

	#endif //USE_OVERLAY

	ambientColor = vec3(0.0);
	lightColor = var_Color.rgb;
	attenuation = 1.0;

	#if defined(USE_LIGHTMAP)
		float lmBrightMult = clamp(1.0 - (length(lightmapColor.rgb) / 3.0), 0.0, 0.9);
		lmBrightMult *= lmBrightMult * 0.7;
		lightColor	= lightmapColor.rgb * lmBrightMult * var_Color.rgb;
	#endif


	#if defined(USE_SHADOWMAP)

		vec2 shadowTex = gl_FragCoord.xy * r_FBufScale;
		float shadowValue = texture2D(u_ShadowMap, shadowTex).r;

	#endif //defined(USE_SHADOWMAP) 



	#if defined(USE_LIGHTMAP)

		ambientColor = lightColor;
		float surfNL = clamp(-dot(var_PrimaryLightDir.xyz, N.xyz/*m_Normal.xyz*/), 0.0, 1.0);
		lightColor /= max(surfNL, 0.25);
		ambientColor = clamp(ambientColor - lightColor * surfNL, 0.0, 1.0);

	#endif //defined(USE_LIGHTMAP)
  

		#if defined(USE_LIGHTMAP)
			lightColor *= lightmapColor.rgb;
		#endif


		gl_FragColor = vec4 (diffuse.rgb * lightColor, diffuse.a * var_Color.a);

		if (u_Local1.a == 19 || u_Local1.a == 20)
		{// Tree billboards... Need to match tree colors...
			gl_FragColor.rgb = pow(gl_FragColor.rgb, vec3(1.3));
		}

		//specular.a = (1.0 - norm.a);
		specular.a = ((clamp(u_Local1.g, 0.0, 1.0) + clamp(u_Local3.a, 0.0, 1.0)) / 2.0) * 1.6;

		#if defined(USE_SPECULARMAP)
		if (u_Local1.g != 0.0)
		{// Real specMap...
			specular = texture2D(u_SpecularMap, texCoords);
		}
		else
		#endif //defined(USE_SPECULARMAP)
		{// Fake it...
			specular.rgb = gl_FragColor.rgb;
		}

		#if defined(USE_GAMMA2_TEXTURES)
			specular.rgb *= specular.rgb;
		#endif //defined(USE_GAMMA2_TEXTURES)

		specular.rgb *= u_SpecularScale.rgb;



	#if defined(USE_CUBEMAP)
		if (u_Local3.a > 0.0 && u_EnableTextures.w > 0.0 && u_CubeMapStrength > 0.0) 
		{
			float spec = 0.0;

			NE = clamp(dot(m_Normal.xyz/*N*/, E), 0.0, 1.0);

			vec3 reflectance = EnvironmentBRDF(clamp(specular.a, 0.5, 1.0) * 100.0, NE, specular.rgb);
			vec3 R = reflect(E, m_Normal.xyz/*N*/);
			vec3 parallax = u_CubeMapInfo.xyz + u_CubeMapInfo.w * viewDir;
			vec3 cubeLightColor = textureCubeLod(u_CubeMap, R + parallax, 7.0 - specular.a * 7.0).rgb * u_EnableTextures.w * 0.25; //u_Local3.a
			gl_FragColor.rgb += (cubeLightColor * reflectance * (u_Local3.a * specular.a)) * u_CubeMapStrength;
		}
	#endif


	#if defined(USE_SHADOWMAP)
		//gl_FragColor.rgb *= clamp(shadowValue + 0.5, 0.0, 1.0);
		gl_FragColor.rgb *= clamp(shadowValue, 0.4, 1.0);
	#endif //defined(USE_SHADOWMAP)


	#ifdef USE_SUBSURFACE_SCATTER

		#if defined(USE_PRIMARY_LIGHT) || defined(USE_PRIMARY_LIGHT_SPECULAR)

			// Let's add some sub-surface scatterring shall we???
			if (/*MaterialThickness > 0.0 ||*/ u_Local1.a == 20) // tree leaves
			{
				gl_FragColor.rgb += subScatterFS(gl_FragColor, diffuse, var_PrimaryLightDir.xyz, u_PrimaryLightColor.xyz, E, N, texCoords).rgb;
				gl_FragColor = clamp(gl_FragColor, 0.0, 1.0);
			}

		#endif //defined(USE_PRIMARY_LIGHT) || defined(USE_PRIMARY_LIGHT_SPECULAR)

	#endif //USE_SUBSURFACE_SCATTER


	#if defined(EXPERIMENTAL_LIGHT)
		if (u_Local6.r > 0.0)
		{
			float lightFactor = u_Local5.b;

			vec3 lightDir = u_PrimaryLightOrigin.xyz - m_vertPos.xyz;
			float lightStrength = clamp(1.0 - (length(lightDir) * (1.0 / distance(u_PrimaryLightOrigin.xyz, m_vertPos.xyz))), 0.0, 1.0) * 0.5;

			gl_FragColor.rgb += ExperimentalLighting(m_vertPos.xyz, u_ViewOrigin.xyz, u_PrimaryLightOrigin.xyz, DETAILED_NORMAL.xyz, u_PrimaryLightColor.rgb, lightStrength, gl_FragColor.rgb);

			for (int li = 0; li < u_lightCount; li++)
			{
				lightDir = u_lightPositions2[li] - m_vertPos.xyz;
				lightStrength = clamp(1.0 - (length(lightDir) * (1.0 / u_lightDistances[li])), 0.0, 1.0) * 0.5;
				gl_FragColor.rgb += ExperimentalLighting(m_vertPos.xyz, u_ViewOrigin.xyz, u_lightPositions2[li].xyz, DETAILED_NORMAL.xyz, u_lightColors[li].rgb, lightStrength, gl_FragColor.rgb);
			}
		}
	#elif defined(USE_PRIMARY_LIGHT) || defined(USE_PRIMARY_LIGHT_SPECULAR)
		if (u_Local6.r > 0.0)
		{
			float lambertian2 = dot(var_PrimaryLightDir.xyz,DETAILED_NORMAL.xyz/*N*/);
			float spec2 = 0.0;
			bool noSunPhong = false;
			float phongFactor = u_Local5.b;

			if (phongFactor < 0.0)
			{// Negative phong value is used to tell terrains not to use sunlight (to hide the triangle edge differences)
				noSunPhong = true;
				phongFactor = 0.0;
			}

			if(lambertian2 > 0.0)
			{// this is blinn phong
				vec3 halfDir2 = normalize(var_PrimaryLightDir.xyz + E);
				float specAngle = max(dot(halfDir2, DETAILED_NORMAL.xyz/*N*/), 0.0);
				spec2 = pow(specAngle, 16.0);
				gl_FragColor.rgb += vec3(spec2 * (1.0 - specular.a)) * gl_FragColor.rgb * u_PrimaryLightColor.rgb * phongFactor;
			}

			if (noSunPhong)
			{// Invert phong value so we still have non-sun lights...
				phongFactor = -u_Local5.b;
			}

			for (int li = 0; li < u_lightCount; li++)
			{
				vec3 lightDir = u_lightPositions2[li] - m_vertPos.xyz;
				float lambertian3 = dot(lightDir.xyz,DETAILED_NORMAL.xyz/*N*/);
				float spec3 = 0.0;

				if(lambertian3 > 0.0)
				{
					float lightStrength = clamp(1.0 - (length(lightDir) * (1.0 / u_lightDistances[li])), 0.0, 1.0) * 0.5;

					if(lightStrength > 0.0)
					{// this is blinn phong
						vec3 halfDir3 = normalize(lightDir.xyz + E);
						float specAngle3 = max(dot(halfDir3, DETAILED_NORMAL.xyz/*N*/), 0.0);
						spec3 = pow(specAngle3, 16.0);
						gl_FragColor.rgb += vec3(spec3 * (1.0 - specular.a)) * u_lightColors[li].rgb * lightStrength * phongFactor;
					}
				}
			}
		}
	#endif //defined(EXPERIMENTAL_LIGHT)

	//gl_FragColor.rgb = N.xyz * 0.5 + 0.5;

	#if defined(USE_GLOW_BUFFER)
		out_Glow = gl_FragColor;
	#else
		out_Glow = vec4(0.0);
	#endif

	out_Normal = vec4(DETAILED_NORMAL.xyz * 0.5 + 0.5, specular.a / 8.0);
	out_Position = vec4(m_vertPos, u_Local1.a / MATERIAL_LAST);
}
