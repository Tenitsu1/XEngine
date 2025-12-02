#version 460 core

in vec2 TexCoords; // 從頂點著色器傳入的紋理坐標
out vec4 FragColor;

uniform sampler2D screenTexture; // 屏幕渲染結果的紋理
const vec2 u_resolution = vec2(1280, 720);       // 屏幕解析度

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

vec3 gaussianBlur5x5(vec2 texelSize) {
    // 5x5 高斯模糊
    float kernel[25] = float[](
        1,  4,  7,  4, 1,
        4, 16, 26, 16, 4,
        7, 26, 41, 26, 7,
        4, 16, 26, 16, 4,
        1,  4,  7,  4, 1
    );
    float weight = 0.0;
    vec3 sum = vec3(0.0);
    int idx = 0;
    for (int y = -2; y <= 2; y++) {
        for (int x = -2; x <= 2; x++) {
            float k = kernel[idx++];
            vec2 offset = vec2(x, y) * texelSize;
            vec3 samp = texture(screenTexture, TexCoords + offset).rgb;

            sum += samp * k;
            weight += k;
        }
    }
    return sum / weight;
}

float laplacianEdge(vec2 texelSize) {
    // Laplacian 邊緣檢測
    float kernel[9] = float[](
        0,  1, 0,
        1, -4, 1,
        0,  1, 0
    );
    float sum = 0.0;
    int idx = 0;
    for (int y = -1; y <= 1; ++y)
        for (int x = -1; x <= 1; ++x) {
            vec2 offset = vec2(x, y) * texelSize;
            vec3 samp = texture(screenTexture, TexCoords + offset).rgb;

            float luma = luminance(samp);
            sum += kernel[idx++] * luma;
        }
    return abs(sum);
}

float bilateral(vec2 v, float sigma) {
    // 雙邊濾波輔助函數
    return exp(-dot(v, v) / (2.0 * sigma * sigma));
}

float bilateral(vec3 v, float sigma) {
    // 雙邊濾波輔助函數
    return exp(-dot(v, v) / (2.0 * sigma * sigma));
}

vec3 bilateralFilter(vec2 texelSize, float sigma_s, float sigma_r) {
    // 雙邊濾波
    vec3 center = texture(screenTexture, TexCoords).rgb;
    float weightSum = 0.0;
    vec3 result = vec3(0.0);

    for (int y = -3; y <= 3; ++y) {
        for (int x = -3; x <= 3; ++x) {
            vec2 offset = vec2(x, y) * texelSize;
            vec3 samp = texture(screenTexture, TexCoords + offset).rgb;
            vec3 diff = samp - center;

            float spatial = bilateral(offset, sigma_s);
            float range = bilateral(diff, sigma_r);
            float weight = spatial * range;

            result += samp * weight;
            weightSum += weight;
        }
    }
    return result / weightSum;
}

PixelColor getPixelColors(vec2 texelSize) {
	// 獲取當前像素和周圍像素的顏色
	PixelColor pc;
	vec2 MAX = vec2(1.0);
	vec2 MIN = vec2(0.0);
	vec2 north, south, east, west;

	north = clamp(TexCoords + vec2(0.0, texelSize.y), MIN, MAX);
	south = clamp(TexCoords + vec2(0.0, -texelSize.y), MIN, MAX);
	east = clamp(TexCoords + vec2(texelSize.x, 0.0), MIN, MAX);
	west = clamp(TexCoords + vec2(-texelSize.x, 0.0), MIN, MAX);

	pc.color = texture(screenTexture, TexCoords).rgb;
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
    vec3 color;

    // 直接顯示
    color = texture(screenTexture, TexCoords).rgb;

    // 高斯模糊
    // color = gaussianBlur5x5(texelSize);

    // Laplacian 邊緣檢測 + 雙邊濾波
    float threshold = 0.1;
    float edgeStrength = laplacianEdge(texelSize);
    if (edgeStrength > threshold) {
    // 對邊緣做雙邊濾波
    color = bilateralFilter(texelSize, 2.0, 0.1);
    } else {
    color = texture(screenTexture, TexCoords).rgb;
    }

    // 梯度模糊
    // PixelColor pc = getPixelColors(texelSize);
    // PixelLuminance pl = getPixelLuminance(pc);
    // color = getFinalColor(pc, pl);

	// 雙邊濾波
	// color = bilateralFilter(texelSize, 2.0, 0.1);

    // gamma 校正
    color = pow(color, vec3(1.0 / 2.2));

    // 輸出最終顏色
    FragColor = vec4(color, 1.0);
}