#version 460 core

in vec2 TexCoords; // 從頂點著色器傳入的紋理坐標
out vec4 FragColor;

uniform sampler2D screenTexture; // 屏幕渲染結果的紋理
uniform vec2 u_resolution;       // 屏幕解析度

struct PixelColor {
	vec3 color;
	vec3 north;
	vec3 south;
	vec3 east;
	vec3 west;
};

struct PixelLuminance {
	float luma;
	float lumaNorth;
	float lumaSouth;
	float lumaEast;
	float lumaWest;
};

float luminance(vec3 color) {
	// 計算顏色的亮度
	return dot(color, vec3(0.299, 0.587, 0.114));
}

PixelColor getPixelColors(vec2 texCoords, vec2 texelSize) {
	// 獲取當前像素和周圍像素的顏色
	PixelColor pc;
	vec2 MAX = vec2(1.0);
	vec2 MIN = vec2(0.0);
	vec2 north, south, east, west;

	north = clamp(texCoords + vec2(0.0, texelSize.y), MIN, MAX);
	south = clamp(texCoords + vec2(0.0, -texelSize.y), MIN, MAX);
	east = clamp(texCoords + vec2(texelSize.x, 0.0), MIN, MAX);
	west = clamp(texCoords + vec2(-texelSize.x, 0.0), MIN, MAX);

	pc.color = texture(screenTexture, texCoords).rgb;
	pc.north = texture(screenTexture, north).rgb;
	pc.south = texture(screenTexture, south).rgb;
	pc.east = texture(screenTexture, east).rgb;
	pc.west = texture(screenTexture, west).rgb;
	return pc;
}

PixelLuminance getPixelLuminance(PixelColor pc) {
	// 計算邊緣檢測的亮度梯度
	PixelLuminance pl;
	pl.luma = luminance(pc.color);
	pl.lumaNorth = luminance(pc.north);
	pl.lumaSouth = luminance(pc.south);
	pl.lumaEast = luminance(pc.east);
	pl.lumaWest = luminance(pc.west);
	return pl;
}

bool isEdgeDetected(PixelLuminance pl) {
	float edgeHorizontal = abs(pl.lumaEast - pl.lumaWest);
	float edgeVertical = abs(pl.lumaNorth - pl.lumaSouth);
	return (edgeHorizontal > 0.1 || edgeVertical > 0.1);
}

vec3 getFinalColor(PixelColor pc, PixelLuminance pl) {
	// 如果檢測到邊緣，進行模糊處理
	if (isEdgeDetected(pl)) {
		return (pc.north + pc.south + pc.east + pc.west + pc.color) / 5.0;
	}
	return pc.color;
}

void main() {
    vec2 texelSize = 1.0 / u_resolution; // 紋理像素大小

    PixelColor pc = getPixelColors(TexCoords, texelSize);
    PixelLuminance pl = getPixelLuminance(pc);

    // 根據邊緣檢測結果決定最終顏色
    vec3 color = getFinalColor(pc, pl);

    FragColor = vec4(color, 1.0);
}