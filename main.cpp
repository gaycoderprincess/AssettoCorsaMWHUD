#include <windows.h>
#include <format>
#include <codecvt>
#include <filesystem>
#include "toml++/toml.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "nya_commonhooklib.h"

#include "nya_commonmath.h"

#include "nya_commontimer.cpp"

#include "ac.h"

void OnPluginGUI();
#include "util.h"

#include "chloemwphysics.h"

enum eRPMTexture {
	RPM_7K,
	RPM_8K,
	RPM_9K,
	RPM_10K,
};

eRPMTexture ChooseMaxRpmTextureNumber(float maxRpm) {
	if (maxRpm < 7000.0) return RPM_7K;
	if (maxRpm < 8000.0) return RPM_8K;
	if (maxRpm < 9000.0) return RPM_9K;
	return RPM_10K;
}

float ChooseMaxRpm(float maxRpm) {
	if (maxRpm < 7000.0) return 7000.0;
	if (maxRpm < 8000.0) return 8000.0;
	if (maxRpm < 9000.0) return 9000.0;
	return 10000.0;
}

float CalcAngleForRPM(float rpm, float redline) {
	float v2 = rpm / ChooseMaxRpm(redline);
	if (v2 >= 0.0) {
		if (v2 > 1.0) {
			v2 = 1.0;
		}
	}
	else {
		v2 = 0.0;
	}
	float result = v2 * 228.0 + 66.0;
	if (result > 360.0) {
		result = result - 360.0;
	}
	if (result < 0.0) {
		return 360.0 - result;
	}
	return result;
}

struct tDrawable {
	const char* name;
	float posx;
	float posy;
	float posz;
	float sizex;
	float sizey;
	NyaDrawing::CNyaRGBA32 color;
	float rotation = 0.0;
	const char* texName;
	Texture* texture;
	float animTime = 0.0;
	float storedValue = 0.0;

	void Render(double delta) {
		if (!strcmp(name, "tach_fill")) texName = "plugins/CustomHUD/TACH_FILL_00.png";
		if (!strcmp(name, "n20_icon")) texName = "plugins/CustomHUD/N20_ICON.png";
		if (!strcmp(name, "880D0570")) texName = "plugins/CustomHUD/PERSUIT_ICON.png";
		if (!strcmp(name, "TAC_Lines_7500")) {
			switch (ChooseMaxRpmTextureNumber(pMyPlugin->car->drivetrain.acEngine.defaultEngineLimiter)) {
				case RPM_7K:
					texName = "plugins/CustomHUD/7000_LINES_00.png";
					break;
				case RPM_8K:
					texName = "plugins/CustomHUD/8000_LINES_00.png";
					break;
				case RPM_9K:
					texName = "plugins/CustomHUD/9000_LINES_00.png";
					break;
				case RPM_10K:
				default:
					texName = "plugins/CustomHUD/10000_LINES_00.png";
					break;
			}
		}
		if (!strcmp(name, "Turbo_Lines")) texName = "plugins/CustomHUD/TURBO_LINES_00.png";
		if (!strcmp(name, "Shift_light")) texName = "plugins/CustomHUD/SHIFT_UP_ICON.png";
		if (!strcmp(name, "heat_icon")) texName = "plugins/CustomHUD/HEAT_ICON.png";
		if (!strcmp(name, "10E50F5E")) texName = "plugins/CustomHUD/HEAT_ICON_ADDITIVE.png";
		if (!strcmp(name, "8B8CCA96")) texName = "plugins/CustomHUD/METER_BACKING2.png";
		if (!strcmp(name, "DD77F899")) texName = "plugins/CustomHUD/METER_BACKING2.png";
		if (!strcmp(name, "3rdPersonNeedle")) texName = "plugins/CustomHUD/TACH_NEEDLE_00.png";
		if (!strcmp(name, "3rdperson_TurboDial")) texName = "plugins/CustomHUD/TURBO_NEEDLE_00.png";
		if (!strcmp(name, "basepoly_add")) texName = "plugins/CustomHUD/SHIFT_UP_ICON.png"; // nos arrow
		if (!strcmp(name, "basepoly")) texName = "plugins/CustomHUD/SHIFT_UP_ICON.png"; // heat arrow
		if (!strcmp(name, "88132592")) texName = "plugins/CustomHUD/SHIFT_UP_ICON.png"; // speedbreaker arrow

		if (!texture && texName) texture = LoadTexture(texName);

		auto tmpColor = color;

		auto fuel = pMyPlugin->car->fuel / pMyPlugin->car->maxFuel;
		if (fuel <= 0.0101) fuel = 0.0;

		if (!strcmp(name, "basepoly_add")) {
			if (fuel > 0.0) {
				tmpColor = {191, 238, 65, 251};
			}
			else {
				tmpColor = {255, 255, 255, 60};
			}
		}

		if (!strcmp(name, "n20_icon")) {
			if (fuel > 0.0) {
				// burning nos animation
				if (ChloeMWPhysics::IsNOSEnabled(pMyPlugin->car) && fuel < storedValue) {
					animTime += delta;
					if (animTime >= 0.360) {
						animTime -= 0.360;
					}
					if (animTime > 0.180) {
						float f = (animTime - 0.180) / 0.180;
						tmpColor.r = std::lerp(255, 194, f);
						tmpColor.g = std::lerp(255, 242, f);
						tmpColor.b = std::lerp(255, 66, f);
						tmpColor.a = 255;

						sizex = std::lerp(36, 30, f);
						sizey = std::lerp(36, 30, f);
					}
					else {
						float f = animTime / 0.180;
						tmpColor.r = std::lerp(194, 255, f);
						tmpColor.g = std::lerp(242, 255, f);
						tmpColor.b = std::lerp(66, 255, f);
						tmpColor.a = 255;

						sizex = std::lerp(30, 36, f);
						sizey = std::lerp(30, 36, f);
					}
				}
				else {
					tmpColor = {194, 242, 66, 255};
				}
			}
			else {
				tmpColor = {255, 255, 255, 60};
			}
			storedValue = fuel;
		}

		float f = 480 * (16.0 / 9.0);
		float x1 = (posx - (sizex*0.5)) / f;
		float x2 = (posx + (sizex*0.5)) / f;
		float y1 = (posy - (sizey*0.5)) / 480.0;
		float y2 = (posy + (sizey*0.5)) / 480.0;

		x1 += 0.5;
		x2 += 0.5;
		y1 += 0.5;
		y2 += 0.5;

		if (!strcmp(name, "3rdperson_TurboDial") || !strcmp(name, "Turbo_Lines")) {
			if (!ChloeMWPhysics::HasTurbo(pMyPlugin->car)) return;
		}

		if (!strcmp(name, "880D0570") || !strcmp(name, "88132592")) {
			if (ChloeMWPhysics::GetGameBreakerCharge(pMyPlugin->car) > 0.0) {
				tmpColor = {251, 192, 114, 251};
			}
			else {
				tmpColor = {255, 255, 255, 60};
			}
		}

		if (!strcmp(name, "3rdPersonNeedle")) {
			float rpm = pMyPlugin->car->drivetrain.acEngine.lastInput.rpm;
			float maxRpm = pMyPlugin->car->drivetrain.acEngine.defaultEngineLimiter;
			DrawRectangle(x1, x2, y1, y2, tmpColor, 0.0, texture, CalcAngleForRPM(rpm, maxRpm) * 0.01745329);
		}
		else if (!strcmp(name, "3rdperson_TurboDial")) {
			DrawRectangle(x1, x2, y1, y2, tmpColor, 0.0, texture, -((ChloeMWPhysics::GetInductionPSI(pMyPlugin->car) - -20.0) * 0.025 * 90.0 - 45.0) * 0.01745329);
		}
		else {
			DrawRectangle(x1, x2, y1, y2, tmpColor, 0.0, texture, rotation * 0.01745329);
		}

		/*if (!texture) {
			tNyaStringData str;
			str.x = x1;
			str.y = y2;
			str.size = 0.03;

			if (str.x < 0) str.x = 0.5;
			if (str.y < 0) str.y = 0.5;
			if (str.x > 1) str.x = 0.5;
			if (str.y > 1) str.y = 0.5;

			DrawString(str, name);
		}*/
	}
};
tDrawable aDrawables[] = {
		{"tach_fill", 341.00, 141.00, 355.91, 140.00, 140.00, {252, 193, 114, 252}},
		{"8B8CCA96", 340.00, 140.00, 345.91, 152, 150, {252, 252, 252, 59}, -40.0}, // Meter_Backing2.tga speedbreaker bg
		{"DD77F899", 341.50, 141.50, 320.00, 152.0, 150.0, {252, 252, 252, 59}, 140.0}, // Meter_Backing2.tga nos bg
		{"n20_icon", 275.50, 205.50, 219.00, 30.00, 30.00, {191, 238, 65, 251}},
		{"880D0570", 407.00, 75.00, 187.96, 28.00, 29.00, {251, 192, 114, 251}}, // Persuit_Icon.tga
		{"88132592", 396.00, 86.00, 187.96, 9.0, 9.0, {251, 192, 114, 251}, 180.0 + 45.0}, // speedbreaker arrow
		{"TAC_Lines_7500", 341.00, 139.00, 185.91, 133.00, 128.00, {252, 252, 252, 252}},
		{"Turbo_Lines", 341.00, 190.00, 175.91, 128.00, 32.00, {252, 252, 252, 252}},
		{"Shift_light", 335.00, 120.00, 175.91, 12.00, 12.00, {0, 0, 0, 127}},
		{"3rdperson_TurboDial", 340.00, 141.00, 165.91, 32.00, 136.00, {252, 193, 114, 252}},
		{"3rdPersonNeedle", 341.00, 140.00, 155.91, 32.0, 128.0, {252, 193, 114, 252}},
		{"basepoly_add", 286.00, 195.00, 109.91, 9.0, 9.0, {191, 238, 65, 251}, 45.0}, // nos arrow
};

void DrawCustomArc(float x, float y, float sizex, float sizey, float angle, float arc, int numSegments, NyaDrawing::CNyaRGBA32 rgb = {255,255,255,255}, Texture* texture = nullptr) {
	if (arc <= 0.0) return;

	float minX = x - (sizex*0.5);
	float minY = y - (sizey*0.5);
	float maxX = x + (sizex*0.5);
	float maxY = y + (sizey*0.5);

	for (int i = 0; i < numSegments; i++) {
		float fCurr = (i / (double)numSegments) * arc * std::numbers::pi*2;
		float fNext = ((i + 1) / (double)numSegments) * arc * std::numbers::pi*2;
		fCurr += angle;
		fNext += angle;
		float x1 = x + (std::sin(fCurr) * sizex * 0.5);
		float y1 = y + (std::cos(fCurr) * sizey * 0.5);
		float x2 = x + (std::sin(fNext) * sizex * 0.5);
		float y2 = y + (std::cos(fNext) * sizey * 0.5);
		DrawTriangle(x1, y1, x2, y2, x, y, rgb, 0, 0, 1, 1, texture, minX, minY, maxX, maxY);
	}
}

void DrawNOSBar(float nosFill) {
	float nosx = 341.50 / (480.0 * (16.0 / 9.0));
	float nosy = 141.50 / 480.0;
	nosx += 0.5;
	nosy += 0.5;

	float sizex = 152.0 / (480.0 * (16.0 / 9.0));
	float sizey = 150.0 / 480.0;

	NyaDrawing::CNyaRGBA32 nosRgb = {194, 242, 66, 255};

	static auto tex = LoadTexture("plugins/CustomHUD/METER_BACKING2_ROTATE2.png");
	DrawCustomArc(nosx, nosy, sizex, sizey, (270 + 44.0) * 0.01745329, nosFill * 0.477, 32, nosRgb, tex);
}

void DrawGamebreakerBar(float gamebreakerFill) {
	float spdx = 340.00 / (480.0 * (16.0 / 9.0));
	float spdy = 140.00 / 480.0;
	spdx += 0.5;
	spdy += 0.5;

	float sizex = 152.0 / (480.0 * (16.0 / 9.0));
	float sizey = 150.0 / 480.0;

	NyaDrawing::CNyaRGBA32 gameBreakerRgb = {255, 195, 115, 255};

	static auto tex = LoadTexture("plugins/CustomHUD/METER_BACKING2_ROTATE2.png");
	DrawCustomArc(spdx, spdy, sizex, sizey, (270 + 180 + 44.0) * 0.01745329, gamebreakerFill * 0.477, 32, gameBreakerRgb, tex);
}

void DrawSpeedoText() {
	bool bMetric = false;
	static auto units = LoadTexture(bMetric ? "plugins/CustomHUD/text/kmh.png" : "plugins/CustomHUD/text/mph.png");
	if (units) {
		DrawRectangle(0, 1, 0, 1, {255,255,255,255}, 0, units);
	}

	static auto numbers = LoadTexture("plugins/CustomHUD/text/numbersbg.png");
	if (numbers) {
		DrawRectangle(0, 1, 0, 1, {0,0,0,80}, 0, numbers);
	}

	int gear = pMyPlugin->car->drivetrain.currentGear;

	static Texture* gears[] = {
			LoadTexture("plugins/CustomHUD/text/gear_r.png"),
			LoadTexture("plugins/CustomHUD/text/gear_n.png"),
			LoadTexture("plugins/CustomHUD/text/gear_1.png"),
			LoadTexture("plugins/CustomHUD/text/gear_2.png"),
			LoadTexture("plugins/CustomHUD/text/gear_3.png"),
			LoadTexture("plugins/CustomHUD/text/gear_4.png"),
			LoadTexture("plugins/CustomHUD/text/gear_5.png"),
			LoadTexture("plugins/CustomHUD/text/gear_6.png"),
			LoadTexture("plugins/CustomHUD/text/gear_7.png"),
			LoadTexture("plugins/CustomHUD/text/gear_8.png"),
			LoadTexture("plugins/CustomHUD/text/gear_9.png"),
	};
	if (auto tex = gears[gear]) {
		if (pMyPlugin->car->drivetrain.isGearGrinding) {
			DrawRectangle(0, 1, 0, 1, {0,0,0,136}, 0, tex);
		}
		else {
			DrawRectangle(0, 1, 0, 1, {0,0,0,230}, 0, tex);
		}
	}
}

void OnPluginGUI() {
	static CNyaTimer gTimer;
	gTimer.Process();

	for (auto& drawable : aDrawables) {
		drawable.Render(gTimer.fDeltaTime);
		if (!strcmp(drawable.name, "Shift_light")) {
			DrawSpeedoText();
		}
	}

	auto fuel = pMyPlugin->car->fuel / pMyPlugin->car->maxFuel;
	if (fuel <= 0.0101) fuel = 0.0;

	// todo redline!!
	DrawNOSBar(fuel);
	DrawGamebreakerBar(ChloeMWPhysics::GetGameBreakerCharge(pMyPlugin->car));

	// ac test

	/*static auto tex = LoadTexture("plugins/CustomHUD/text/numbersbg.png");

	auto graphics = pMyPlugin->sim->game->graphics;
	auto gl = graphics->gl;
	gl->color4f(1.0, 1.0, 1.0, 1.0);
	if (tex) graphics->setTexture(0, tex);
	gl->quad(0, 0, 1920, 1080, tex != nullptr, nullptr);*/
}

BOOL WINAPI DllMain(HINSTANCE, DWORD fdwReason, LPVOID) {
	switch(fdwReason) {
		case DLL_PROCESS_ATTACH: {
			if (NyaHookLib::GetEntryPoint() != 0x15AE310) {
				MessageBoxA(nullptr, "Unsupported game version! Make sure you're using the latest x64 Steam release (.exe size of 22890776 bytes)", "nya?!~", MB_ICONERROR);
				return TRUE;
			}
		} break;
		default:
			break;
	}
	return TRUE;
}