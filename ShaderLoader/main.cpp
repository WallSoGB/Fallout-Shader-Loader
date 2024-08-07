#include "d3d9.h"
#include "format"
#include "GameData.hpp"
#include "nvse/PluginAPI.h"
#include "SafeWrite.h"

#define EXTERN_DLL_EXPORT extern "C" __declspec(dllexport)

IDebugLog	   gLog("logs\\ShaderLoader.log");

static bool bNVRPresent = false;
static NVSEMessagingInterface* pNVSEMessaging = nullptr;
static PluginHandle uiPluginHandle = 0;

template <typename FUNC>
void* CreateShader(const char* apFilename, FUNC aFunc) {
	DWORD* pShaderData = nullptr;
	bool bFreeMem = false;
	char cPath[MAX_PATH];
	sprintf_s(cPath, "Data\\Shaders\\Loose\\%s", apFilename);
	BSFile* pFile = FileFinder::GetFile(cPath, NiFile::READ_ONLY, 0x4000, ARCHIVE_TYPE_SHADERS);
	if (pFile && pFile->m_bGood) {
		_MESSAGE("Loaded %s", apFilename);
		UInt32 uiSize = 0;

		uiSize = pFile->GetSize();
		pShaderData = static_cast<DWORD*>(MemoryManager::Allocate(uiSize));
		bFreeMem = true;

		pFile->ReadBuffer(pShaderData, uiSize);
		delete pFile;
	}

	if (!pShaderData) {
		BSShaderManager::ShaderPackage* pShaderPkg = BSShaderManager::ShaderPackage::GetSingleton();
		if (pShaderPkg) {
			BSShaderManager::ShaderPackage::RawShader* pRaw = pShaderPkg->GetShader(apFilename);
			if (pRaw)
				pShaderData = &pRaw->shader;
		}
	}

	if (!pShaderData) {
		if (bFreeMem)
			MemoryManager::Deallocate(pShaderData);

		_MESSAGE("Failed to find %s", apFilename);
		return nullptr;
	}

	NiDX9Renderer* pRenderer = BSShaderManager::GetRenderer();
	void* pNiShader = aFunc(pShaderData);

	if (bFreeMem)
		MemoryManager::Deallocate(pShaderData);

	return pNiShader;
}

// 0xBE0FE0
NiD3DVertexShader* __fastcall BSShader::CreateVertexShaderEx(BSShader* apThis, void*, const char* apPath, D3DXMACRO* apMacro, const char* apShaderVersion, const char* apFilename) {
	return (NiD3DVertexShader*)CreateShader(apFilename, [&](DWORD* apShaderData) {
		LPDIRECT3DVERTEXSHADER9 pShader = nullptr;
		NiD3DVertexShader* pVertexShader = nullptr;
		NiDX9Renderer* pRenderer = BSShaderManager::GetRenderer();
		if (SUCCEEDED(pRenderer->GetD3DDevice()->CreateVertexShader(apShaderData, &pShader))) {
			pVertexShader = NiD3DVertexShader::Create(pRenderer);
			pVertexShader->SetShaderHandle(pShader);
			pVertexShader->SetName(apFilename);

			if (bNVRPresent) {
				pVertexShader->bEnabled = true;
				pVertexShader->pShaderHandleBackup = pShader;
			}
		}
		return pVertexShader;
	});
}

// 0xBE1750
NiD3DPixelShader* __fastcall BSShader::CreatePixelShaderEx(BSShader* apThis, void*, const char* apPath, D3DXMACRO* apMacro, const char* apShaderVersion, const char* apFilename) {
	return (NiD3DPixelShader*)CreateShader(apFilename, [&](DWORD* apShaderData) {
		LPDIRECT3DPIXELSHADER9 pShader = nullptr;
		NiD3DPixelShader* pPixelShader = nullptr;
		NiDX9Renderer* pRenderer = BSShaderManager::GetRenderer();
		if (SUCCEEDED(pRenderer->GetD3DDevice()->CreatePixelShader(apShaderData, &pShader))) {
			pPixelShader = NiD3DPixelShader::Create(pRenderer);
			pPixelShader->SetShaderHandle(pShader);
			pPixelShader->SetName(apFilename);

			if (bNVRPresent) {
				pPixelShader->bEnabled = true;
				pPixelShader->pShaderHandleBackup = pShader;
			}
		}
		return pPixelShader;
	});
}

enum ShaderLoaderMessages {
	SL_ShaderRefresh = 0 // Sent on RefreshShaders
};

static void __cdecl BSShaderManager__ReloadShaders() {
	CdeclCall(0xB557D0);
	if (pNVSEMessaging) {
		_MESSAGE("Reloading shaders");
		pNVSEMessaging->Dispatch(uiPluginHandle, SL_ShaderRefresh, nullptr, 0, nullptr);
	}
}

EXTERN_DLL_EXPORT NiD3DPixelShader* CreatePixelShader(const char* apFilename) {
	return BSShader::CreatePixelShaderEx(nullptr, nullptr, nullptr, nullptr, nullptr, apFilename);
}

EXTERN_DLL_EXPORT NiD3DVertexShader* CreateVertexShader(const char* apFilename) {
	return BSShader::CreateVertexShaderEx(nullptr, nullptr, nullptr, nullptr, nullptr, apFilename);
}

EXTERN_DLL_EXPORT bool NVSEPlugin_Query(const NVSEInterface* nvse, PluginInfo* info) {
	info->infoVersion = PluginInfo::kInfoVersion;
	info->name = "Shader Loader";
	info->version = 120;
	return true;
}

EXTERN_DLL_EXPORT bool NVSEPlugin_Load(NVSEInterface* nvse) {
	if (!nvse->isEditor) {
		SafeWrite8(0xB575AA, 0x75); // Prevent shader package destruction to allow shader reloading

		if (!GetModuleHandle("NewVegasReloaded.dll")) {
			WriteRelJump(0xBE0FE0, BSShader::CreateVertexShaderEx);
			WriteRelJump(0xBE1750, BSShader::CreatePixelShaderEx);
		}
		else {
			_MESSAGE("New Vegas Reloaded detected, skipping hooks");
			bNVRPresent = true;
		}

		for (UInt32 uiAddr : {0x5BF43C, 0x5C5A39})
			ReplaceCall(uiAddr, BSShaderManager__ReloadShaders);

		pNVSEMessaging = (NVSEMessagingInterface*)nvse->QueryInterface(kInterface_Messaging);
		uiPluginHandle = nvse->GetPluginHandle();
	}

	return true;
}