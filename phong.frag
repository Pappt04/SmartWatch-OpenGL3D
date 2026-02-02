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
uniform Material uMaterial;
uniform vec3 uViewPos;	//Pozicija kamere (za racun spekularne komponente)
uniform bool uUseTexture;
uniform sampler2D uTexture;

void main()
{
	vec3 resA = uLight.kA * uMaterial.kA;

	vec3 normal = normalize(chNor);
	vec3 lightDirection = normalize(uLight.pos - chFragPos);
	float nD = max(dot(normal, lightDirection), 0.0);
	vec3 resD = uLight.kD * ( nD * uMaterial.kD);

	vec3 viewDirection = normalize(uViewPos - chFragPos);
	vec3 reflectionDirection = reflect(-lightDirection, normal);
	float s = pow(max(dot(viewDirection, reflectionDirection), 0.0), uMaterial.shine);
	vec3 resS = uLight.kS * (s * uMaterial.kS);

	vec3 phongColor = resA + resD + resS;
	
	if (uUseTexture) {
		vec4 texColor = texture(uTexture, chTexCoord);
		outCol = vec4(phongColor, 1.0) * texColor;
	} else {
		outCol = vec4(phongColor, 1.0);
	}
}