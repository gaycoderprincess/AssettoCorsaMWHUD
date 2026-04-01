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
	WriteLog("Mod initialized");
	return true;
}

// todo this isn't exactly the best way
auto GetTrack() {
	return pMyPlugin->car->splineLocator.track;
}