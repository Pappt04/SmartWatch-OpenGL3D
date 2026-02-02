#version 330 core

struct Light{ //Svjetlosni izvor
	vec3 pos; //Pozicija
	vec3 kA; //Ambijentalna komponenta (Indirektno svjetlo)
	vec3 kD; //Difuzna komponenta (Direktno svjetlo)
	vec3 kS; //Spekularna komponenta (Odsjaj)
};
struct Material{ //Materijal objekta
	vec3 kA;
	vec3 kD;
	vec3 kS;
	float shine; //Uglancanost
};

in vec3 chNor;
in vec3 chFragPos;
in vec2 chTexCoord;

out vec4 outCol;

uniform Light uLight;
uniform Light uWatchLight; // Secondary light from watch screen
uniform bool uUseWatchLight; // Whether to use watch screen as light source
uniform Material uMaterial;
uniform vec3 uViewPos;	//Pozicija kamere (za racun spekularne komponente)
uniform bool uUseTexture;
uniform sampler2D uTexture;

vec3 calcLight(Light light, vec3 normal, vec3 viewDir) {
	vec3 ambient = light.kA * uMaterial.kA;

	vec3 lightDir = normalize(light.pos - chFragPos);
	float diff = max(dot(normal, lightDir), 0.0);
	vec3 diffuse = light.kD * (diff * uMaterial.kD);

	vec3 reflectDir = reflect(-lightDir, normal);
	float spec = pow(max(dot(viewDir, reflectDir), 0.0), uMaterial.shine);
	vec3 specular = light.kS * (spec * uMaterial.kS);

	return ambient + diffuse + specular;
}

void main()
{
	vec3 normal = normalize(chNor);
	vec3 viewDirection = normalize(uViewPos - chFragPos);

	// Main light (sun/sky)
	vec3 phongColor = calcLight(uLight, normal, viewDirection);

	// Watch screen light (weak secondary light)
	if (uUseWatchLight) {
		vec3 watchContrib = calcLight(uWatchLight, normal, viewDirection);
		phongColor += watchContrib;
	}

	// Clamp to prevent over-saturation
	phongColor = clamp(phongColor, 0.0, 1.0);

	if (uUseTexture) {
		vec4 texColor = texture(uTexture, chTexCoord);
		outCol = vec4(phongColor, 1.0) * texColor;
	} else {
		outCol = vec4(phongColor, 1.0);
	}
}