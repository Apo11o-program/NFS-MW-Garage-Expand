#pragma once

#include <stdio.h>
#include <string.h>
#include <stdint.h>

static const unsigned int GarageCarLimitMax = 200;
static const unsigned int GarageCareerBaseCount = 25;
static const unsigned int GarageCustomizationBaseCount = 75;
static const unsigned int GarageCareerExtensionCount = GarageCarLimitMax - GarageCareerBaseCount;
static const unsigned int GarageCustomizationExtensionCount = GarageCarLimitMax - GarageCustomizationBaseCount;

struct GarageCareerExtensionRecord
{
	unsigned char data[0x38];
};

struct GarageCustomizationExtensionRecord
{
	unsigned char data[0x198];
};

#pragma pack(push, 1)
struct GarageExtensionFileHeader
{
	char magic[8];
	unsigned int version;
	unsigned int careerCount;
	unsigned int customizationCount;
	char profileName[16];
};
#pragma pack(pop)

static GarageCareerExtensionRecord GarageCareerExtensions[GarageCareerExtensionCount];
static GarageCustomizationExtensionRecord GarageCustomizationExtensions[GarageCustomizationExtensionCount];
static int GarageCarLimit = GarageCarLimitMax;
static bool GarageExtensionWriteErrorShown;

typedef void* (__thiscall* GarageCustomizationAllocator)(void*);
typedef void* (__thiscall* GarageCareerAllocator)(void*);
typedef int (__thiscall* GarageProfileOperation)(void*, unsigned int);

static void ResetGarageExtensionRecords()
{
	memset(GarageCareerExtensions, 0, sizeof(GarageCareerExtensions));
	memset(GarageCustomizationExtensions, 0, sizeof(GarageCustomizationExtensions));

	for (unsigned int i = 0; i < GarageCareerExtensionCount; ++i)
		GarageCareerExtensions[i].data[0] = 0xFF;
	for (unsigned int i = 0; i < GarageCustomizationExtensionCount; ++i)
		GarageCustomizationExtensions[i].data[0x194] = 0xFF;
}

static bool GetGarageProfileName(char* profileName, size_t bufferSize)
{
	if (!profileName || bufferSize < 2)
		return false;

	const char* source = reinterpret_cast<const char*>(0x91C2B8);
	size_t length = 0;
	while (length < 15 && source[length] != '\0')
	{
		profileName[length] = source[length];
		++length;
	}
	profileName[length] = '\0';
	return length != 0;
}

static bool GetGarageExtensionPath(const char* profileName, char* path, size_t pathSize)
{
	if (!profileName || !profileName[0] || !path || pathSize == 0)
		return false;

	HMODULE module = GetModuleHandleA("NFSMWExtraOptions.asi");
	if (!module)
		return false;

	char modulePath[MAX_PATH];
	DWORD length = GetModuleFileNameA(module, modulePath, MAX_PATH);
	if (!length || length >= MAX_PATH)
		return false;

	char* lastSlash = strrchr(modulePath, '\\');
	if (!lastSlash)
		return false;
	*lastSlash = '\0';

	char safeProfile[16];
	size_t profileLength = 0;
	for (size_t i = 0; profileName[i] && profileLength < sizeof(safeProfile) - 1; ++i)
	{
		unsigned char character = static_cast<unsigned char>(profileName[i]);
		if ((character >= 'a' && character <= 'z') ||
			(character >= 'A' && character <= 'Z') ||
			(character >= '0' && character <= '9') ||
			character == '-' || character == '_')
			safeProfile[profileLength++] = static_cast<char>(character);
		else if (character == ' ')
			safeProfile[profileLength++] = '_';
		else
			safeProfile[profileLength++] = '_';
	}
	safeProfile[profileLength] = '\0';

	int written = _snprintf_s(path, pathSize, _TRUNCATE, "%s\\NFSMWExtraOptions_%s.garage", modulePath, safeProfile);
	return written >= 0;
}

static bool LoadGarageExtensionFile()
{
	char profileName[16];
	char path[MAX_PATH];
	ResetGarageExtensionRecords();

	if (!GetGarageProfileName(profileName, sizeof(profileName)))
		return true;
	if (!GetGarageExtensionPath(profileName, path, sizeof(path)))
		return false;

	DWORD attributes = GetFileAttributesA(path);
	if (attributes == INVALID_FILE_ATTRIBUTES)
	{
		DWORD error = GetLastError();
		return error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND;
	}

	FILE* file = NULL;
	if (fopen_s(&file, path, "rb") != 0 || !file)
		return false;

	GarageExtensionFileHeader header = {};
	bool valid = fread(&header, sizeof(header), 1, file) == 1 &&
		memcmp(header.magic, "NFSMWGAR", 8) == 0 &&
		header.version == 1 &&
		header.careerCount == GarageCareerExtensionCount &&
		header.customizationCount == GarageCustomizationExtensionCount &&
		strncmp(header.profileName, profileName, sizeof(header.profileName)) == 0 &&
		fread(GarageCareerExtensions, sizeof(GarageCareerExtensions), 1, file) == 1 &&
		fread(GarageCustomizationExtensions, sizeof(GarageCustomizationExtensions), 1, file) == 1 &&
		fgetc(file) == EOF;
	fclose(file);

	if (!valid)
	{
		ResetGarageExtensionRecords();
		return false;
	}

	for (unsigned int i = 0; i < GarageCareerExtensionCount; ++i)
	{
		unsigned char handle = static_cast<unsigned char>(GarageCareerBaseCount + i);
		if (GarageCareerExtensions[i].data[0] != 0xFF && GarageCareerExtensions[i].data[0] != handle)
			GarageCareerExtensions[i].data[0] = 0xFF;
	}
	for (unsigned int i = 0; i < GarageCustomizationExtensionCount; ++i)
	{
		unsigned char handle = static_cast<unsigned char>(GarageCustomizationBaseCount + i);
		if (GarageCustomizationExtensions[i].data[0x194] != 0xFF &&
			GarageCustomizationExtensions[i].data[0x194] != handle)
			GarageCustomizationExtensions[i].data[0x194] = 0xFF;
	}

	return true;
}

static bool SaveGarageExtensionFile()
{
	char profileName[16];
	char path[MAX_PATH];
	char temporaryPath[MAX_PATH];
	if (!GetGarageProfileName(profileName, sizeof(profileName)))
		return true;
	if (!GetGarageExtensionPath(profileName, path, sizeof(path)))
		return false;

	int written = _snprintf_s(temporaryPath, sizeof(temporaryPath), _TRUNCATE, "%s.tmp", path);
	if (written < 0)
		return false;

	FILE* file = NULL;
	if (fopen_s(&file, temporaryPath, "wb") != 0 || !file)
		return false;

	GarageExtensionFileHeader header = {};
	memcpy(header.magic, "NFSMWGAR", 8);
	header.version = 1;
	header.careerCount = GarageCareerExtensionCount;
	header.customizationCount = GarageCustomizationExtensionCount;
	strncpy_s(header.profileName, sizeof(header.profileName), profileName, _TRUNCATE);

	bool writtenCompletely =
		fwrite(&header, sizeof(header), 1, file) == 1 &&
		fwrite(GarageCareerExtensions, sizeof(GarageCareerExtensions), 1, file) == 1 &&
		fwrite(GarageCustomizationExtensions, sizeof(GarageCustomizationExtensions), 1, file) == 1 &&
		fflush(file) == 0;
	int closeResult = fclose(file);
	if (!writtenCompletely || closeResult != 0)
	{
		DeleteFileA(temporaryPath);
		return false;
	}

	if (!MoveFileExA(temporaryPath, path, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
	{
		DeleteFileA(temporaryPath);
		return false;
	}

	return true;
}

static void ReportGarageExtensionLoadError()
{
	MessageBoxA(NULL,
		"Could not read the extended garage data file. The original game save was left untouched; cars that use extended garage slots may be incomplete.",
		"NFSMW Extra Options - Garage Expansion",
		MB_OK | MB_ICONWARNING);
}

static void ReportGarageExtensionWriteError()
{
	if (!GarageExtensionWriteErrorShown)
	{
		MessageBoxA(NULL,
			"Could not save the extended garage data file. The original game save was written, but cars using extended garage slots will not be safe to load next time.",
			"NFSMW Extra Options - Garage Expansion",
			MB_OK | MB_ICONERROR);
		GarageExtensionWriteErrorShown = true;
	}
}

static void* __fastcall GetGarageCustomizationRecord(void* database, void*, unsigned char handle)
{
	if (handle < GarageCustomizationBaseCount)
		return reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(database) + 0xFA0 + handle * 0x198);
	if (handle >= static_cast<unsigned int>(GarageCarLimit))
		return NULL;

	GarageCustomizationExtensionRecord* record = &GarageCustomizationExtensions[handle - GarageCustomizationBaseCount];
	if (record->data[0x194] == 0xFF)
		return NULL;
	return record->data;
}

static void* __fastcall GetGarageCareerRecord(void* database, void*, unsigned char handle)
{
	if (handle < GarageCareerBaseCount)
	{
		unsigned char* record = reinterpret_cast<unsigned char*>(
			reinterpret_cast<uintptr_t>(database) + 0x8728 + handle * 0x38);
		return record[0] == 0xFF ? NULL : record;
	}
	if (handle >= static_cast<unsigned int>(GarageCarLimit))
		return NULL;

	GarageCareerExtensionRecord* record = &GarageCareerExtensions[handle - GarageCareerBaseCount];
	if (record->data[0] == 0xFF)
		return NULL;
	return record->data;
}

static void* __fastcall AllocateGarageCustomization(void* database, void*)
{
	GarageCustomizationAllocator original = reinterpret_cast<GarageCustomizationAllocator>(0x580CD0);
	void* record = original(database);
	if (record)
		return record;

	for (unsigned int page = 0; page < 2; ++page)
	{
		GarageCustomizationExtensionRecord* first = &GarageCustomizationExtensions[page * GarageCustomizationBaseCount];
		void* fakeDatabase = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(first->data) - 0xFA0);
		record = original(fakeDatabase);
		if (record)
		{
			unsigned int localIndex = static_cast<unsigned int>(
				(reinterpret_cast<uintptr_t>(record) - reinterpret_cast<uintptr_t>(first->data)) / 0x198);
			unsigned int handle = GarageCustomizationBaseCount + page * GarageCustomizationBaseCount + localIndex;
			if (handle >= static_cast<unsigned int>(GarageCarLimit))
			{
				static_cast<unsigned char*>(record)[0x194] = 0xFF;
				return NULL;
			}
			static_cast<unsigned char*>(record)[0x194] = static_cast<unsigned char>(handle);
			return record;
		}
	}

	return NULL;
}

static void* __fastcall AllocateGarageCareerRecord(void* database, void*)
{
	GarageCareerAllocator original = reinterpret_cast<GarageCareerAllocator>(0x580D60);
	void* record = original(database);
	if (record)
		return record;

	for (unsigned int page = 0; page < GarageCareerExtensionCount / GarageCareerBaseCount; ++page)
	{
		GarageCareerExtensionRecord* first = &GarageCareerExtensions[page * GarageCareerBaseCount];
		void* fakeDatabase = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(first->data) - 0x8728);
		record = original(fakeDatabase);
		if (record)
		{
			unsigned int localIndex = static_cast<unsigned int>(
				(reinterpret_cast<uintptr_t>(record) - reinterpret_cast<uintptr_t>(first->data)) / 0x38);
			unsigned int handle = GarageCareerBaseCount + page * GarageCareerBaseCount + localIndex;
			if (handle >= static_cast<unsigned int>(GarageCarLimit))
			{
				static_cast<unsigned char*>(record)[0] = 0xFF;
				return NULL;
			}
			static_cast<unsigned char*>(record)[0] = static_cast<unsigned char>(handle);
			return record;
		}
	}

	return NULL;
}

static int __fastcall LoadGarageProfile(void* self, void*, unsigned int argument)
{
	GarageProfileOperation original = reinterpret_cast<GarageProfileOperation>(0x5268B0);
	int result = original(self, argument);
	if (!LoadGarageExtensionFile())
		ReportGarageExtensionLoadError();
	return result;
}

static int __fastcall SaveGarageProfile(void* self, void*, unsigned int argument)
{
	GarageProfileOperation original = reinterpret_cast<GarageProfileOperation>(0x5188F0);
	int result = original(self, argument);
	if (SaveGarageExtensionFile())
		GarageExtensionWriteErrorShown = false;
	else
		ReportGarageExtensionWriteError();
	return result;
}

static void __declspec(naked) GarageCarCreationLimitCodeCave()
{
	__asm
	{
		cmp eax, dword ptr [GarageCarLimit]
		jl can_create
		xor eax, eax
		pop esi
		ret 4
	can_create:
		push 0599D38h
		ret
	}
}

static void __declspec(naked) GarageCarLotLimitCodeCave()
{
	__asm
	{
		cmp eax, dword ptr [GarageCarLimit]
		jl can_purchase
		push 07C1A7Ah
		ret
	can_purchase:
		push 07C1AA9h
		ret
	}
}

static void InstallGarageExpansion()
{
	ResetGarageExtensionRecords();

	injector::MakeJMP(0x56F100, GetGarageCustomizationRecord, true);
	injector::MakeJMP(0x56F120, GetGarageCareerRecord, true);
	injector::MakeCALL(0x590C68, AllocateGarageCustomization, true);
	injector::MakeCALL(0x599D52, AllocateGarageCareerRecord, true);

	injector::MakeJMP(0x599D2D, GarageCarCreationLimitCodeCave, true);
	injector::MakeJMP(0x7C1A75, GarageCarLotLimitCodeCave, true);
	injector::WriteMemory<unsigned char>(0x599D5F, static_cast<unsigned char>(GarageCarLimit - 1), true);

	injector::MakeCALL(0x526C0A, LoadGarageProfile, true);
	injector::MakeCALL(0x526E35, LoadGarageProfile, true);
	injector::MakeCALL(0x55A256, LoadGarageProfile, true);
	injector::MakeCALL(0x562E48, LoadGarageProfile, true);
	injector::MakeCALL(0x563756, LoadGarageProfile, true);
	injector::MakeCALL(0x5647D4, LoadGarageProfile, true);
	injector::MakeCALL(0x526854, SaveGarageProfile, true);
	injector::MakeCALL(0x544488, SaveGarageProfile, true);
}
