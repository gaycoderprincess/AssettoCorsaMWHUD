void WriteLog(const std::string& str) {
	static auto file = std::ofstream("plugins/AssettoCorsaMWHUD_gcp.log");

	file << str;
	file << "\n";
	file.flush();
}

extern "C" __declspec(dllexport) bool __fastcall acpGetName(wchar_t* out) { wcscpy_s(out, 256, L"AssettoCorsaMWHUD"); return true; }
extern "C" __declspec(dllexport) bool __fastcall acpShutdown() { return true; }
extern "C" __declspec(dllexport) bool __fastcall acpOnGui(void*) { OnPluginGUI(); return true; }
extern "C" __declspec(dllexport) bool __fastcall acpGetControls(void*) { return false; }
extern "C" __declspec(dllexport) bool __fastcall acpUpdate(void*, float dT) { return true; }

ACPlugin* pMyPlugin = nullptr;
extern "C" __declspec(dllexport) bool __fastcall acpInit(ACPlugin* plugin) {
	pMyPlugin = plugin;
	OnPluginStartup();
	WriteLog("Mod initialized");
	return true;
}

// Load from disk into a raw RGBA buffer
Texture* LoadTexture(const char* filename) {
	int width, height;
	auto texData = stbi_load(filename, &width, &height, nullptr, 4);
	if (texData) {
		auto tex = new Texture(texData, width, height, PixelFormat::eRGBA8);
		delete[] texData;
		return tex;
	}
	delete[] texData;
	return nullptr;
}

namespace NyaDrawing {
	// helper rgb class for casting to int32
	class CNyaRGBA32 {
	public:
		uint8_t r = 0;
		uint8_t g = 0;
		uint8_t b = 0;
		uint8_t a = 0;

		operator unsigned int() { return *(unsigned int*)this; }
	};
}

static inline NyaVec3 ImRotate(const NyaVec3& v, float cos_a, float sin_a) {
	return NyaVec3(v.x * cos_a - v.y * sin_a, v.x * sin_a + v.y * cos_a, 0.0);
}

void ShadeVertLinearUV(NyaVec3 pos, NyaVec3 a, NyaVec3 b, NyaVec3 uv_a, NyaVec3 uv_b) {
	auto graphics = pMyPlugin->sim->game->graphics;
	auto gl = graphics->gl;

	const auto size = b - a;
	const auto uv_size = uv_b - uv_a;
	const auto scale = NyaVec3(size.x != 0.0f ? (uv_size.x / size.x) : 0.0f, size.y != 0.0f ? (uv_size.y / size.y) : 0.0f, 0);

	const auto min = NyaVec3(std::min(uv_a.x, uv_b.x), std::min(uv_a.y, uv_b.y), 0.0);
	const auto max = NyaVec3(std::max(uv_a.x, uv_b.x), std::max(uv_a.y, uv_b.y), 0.0);
	gl->texCoord.x = std::clamp(uv_a.x + ((pos.x - a.x) * scale.x), min.x, max.x);
	gl->texCoord.y = std::clamp(uv_a.y + ((pos.y - a.y) * scale.y), min.y, max.y);
}

void DrawTriangle(float x1, float y1, float x2, float y2, float x3, float y3, NyaDrawing::CNyaRGBA32 rgb, float clipMinX = 0, float clipMinY = 0, float clipMaxX = 1, float clipMaxY = 1, Texture* texture = nullptr, float uvX1 = 0, float uvY1 = 0, float uvX2 = 1, float uvY2 = 1) {
	auto graphics = pMyPlugin->sim->game->graphics;
	auto gl = graphics->gl;
	auto resX = graphics->videoSettings.width;
	auto resY = graphics->videoSettings.height;
	gl->color4f(rgb.r / 255.0, rgb.g / 255.0, rgb.b / 255.0, rgb.a / 255.0);
	if (texture) graphics->setTexture(0, texture);

	gl->begin(eGLPrimitiveType::eTriangles, nullptr);
	gl->useTexture = texture != nullptr;

	auto a = NyaVec3(uvX1*resX, uvY1*resY, 0);
	auto b = NyaVec3(uvX2*resX, uvY2*resY, 0);
	auto uv_a = NyaVec3(0, 0, 0);
	auto uv_b = NyaVec3(1, 1, 0);

	ShadeVertLinearUV(NyaVec3(x1 * resX, y1 * resY, 0.0), a, b, uv_a, uv_b);
	gl->vertex3f(x1 * resX, y1 * resY, 0.0);
	ShadeVertLinearUV(NyaVec3(x2 * resX, y2 * resY, 0.0), a, b, uv_a, uv_b);
	gl->vertex3f(x2 * resX, y2 * resY, 0.0);
	ShadeVertLinearUV(NyaVec3(x3 * resX, y3 * resY, 0.0), a, b, uv_a, uv_b);
	gl->vertex3f(x3 * resX, y3 * resY, 0.0);
	gl->end();
}

void ImageRotated(Texture* texture, NyaVec3 center, NyaVec3 size, float angle, NyaDrawing::CNyaRGBA32 col) {
	float cos_a = cosf(angle);
	float sin_a = sinf(angle);
	NyaVec3 pos[4] = {
			center + ImRotate(NyaVec3(-size.x * 0.5f, -size.y * 0.5f, 0.0), cos_a, sin_a),
			center + ImRotate(NyaVec3(+size.x * 0.5f, -size.y * 0.5f, 0.0), cos_a, sin_a),
			center + ImRotate(NyaVec3(+size.x * 0.5f, +size.y * 0.5f, 0.0), cos_a, sin_a),
			center + ImRotate(NyaVec3(-size.x * 0.5f, +size.y * 0.5f, 0.0), cos_a, sin_a)
	};
	NyaVec3 uvs[4] = {
			NyaVec3(0.0f, 0.0f, 0.0f),
			NyaVec3(1.0f, 0.0f, 0.0f),
			NyaVec3(1.0f, 1.0f, 0.0f),
			NyaVec3(0.0f, 1.0f, 0.0f)
	};

	auto graphics = pMyPlugin->sim->game->graphics;
	auto gl = graphics->gl;
	gl->color4f(col.r / 255.0, col.g / 255.0, col.b / 255.0, col.a / 255.0);
	if (texture) graphics->setTexture(0, texture);

	gl->begin(eGLPrimitiveType::eQuads, nullptr);
	gl->useTexture = texture != nullptr;

	gl->texCoord.x = uvs[3].x;
	gl->texCoord.y = uvs[3].y;
	gl->vertex3f(pos[3].x, pos[3].y, 0.0);
	gl->texCoord.x = uvs[2].x;
	gl->texCoord.y = uvs[2].y;
	gl->vertex3f(pos[2].x, pos[2].y, 0.0);
	gl->texCoord.x = uvs[1].x;
	gl->texCoord.y = uvs[1].y;
	gl->vertex3f(pos[1].x, pos[1].y, 0.0);
	gl->texCoord.x = uvs[0].x;
	gl->texCoord.y = uvs[0].y;
	gl->vertex3f(pos[0].x, pos[0].y, 0.0);
	gl->end();
}

void DrawRectangle(float x1, float x2, float y1, float y2, NyaDrawing::CNyaRGBA32 rgb, float rounding = 0, Texture* texture = nullptr, float rotation = 0) {
	auto graphics = pMyPlugin->sim->game->graphics;
	auto gl = graphics->gl;
	auto resX = graphics->videoSettings.width;
	auto resY = graphics->videoSettings.height;
	gl->color4f(rgb.r / 255.0, rgb.g / 255.0, rgb.b / 255.0, rgb.a / 255.0);
	if (texture) graphics->setTexture(0, texture);

	if (rotation == 0.0) {
		gl->quad(x1 * resX, y1 * resY, (x2 - x1) * resX, (y2 - y1) * resY, texture != nullptr, nullptr);
	}
	else {
		NyaVec3 v1;
		v1.x = x1 * resX;
		v1.y = y1 * resY;
		NyaVec3 v2;
		v2.x = x2 * resX;
		v2.y = y2 * resY;

		ImageRotated(texture, NyaVec3((v1.x + v2.x) * 0.5, (v1.y + v2.y) * 0.5, 0), NyaVec3(v2.x - v1.x, v2.y - v1.y, 0), rotation, rgb);
	}
}