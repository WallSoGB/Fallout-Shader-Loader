#include "GameData.hpp"
#include "nvse/PluginAPI.h"
#include "shared/Utils/DebugLog.hpp"

#include <algorithm>
#include <vector>

BS_ALLOCATORS;

IDebugLog	   gLog("logs\\ShaderLoader.log");

static bool								bLoadedPackageOnDemand = false;
static uint32_t							uiShaderCleanupFrameCounter = 0;
static bool								bNVRPresent = false;
static NVSEMessagingInterface*			pNVSEMessaging = nullptr;
static PluginHandle						uiPluginHandle = 0;

static const uint32_t					uiEOFEffectIndexDivider = 1000u;

struct EOFISEffect {
	uint32_t			uiIndex;
	ImageSpaceEffect*	pEffect;
};
static std::vector<EOFISEffect>			kAdditionalEOFEffects;

bool EffectOrder(const EOFISEffect& arEffect1, const EOFISEffect& arEffect2) {
	return arEffect1.uiIndex < arEffect2.uiIndex;
}

template <typename FUNC>
void* CreateShader(const char* apFilename, FUNC aFunc) {
	DWORD* pShaderData = nullptr;
	bool bFreeMem = false;
	char cPath[MAX_PATH];
	sprintf_s(cPath, "Data\\Shaders\\Loose\\%s", apFilename);
	BSFile* pFile = FileFinder::GetFile(cPath, NiFile::READ_ONLY, 0x4000, ARCHIVE_TYPE_SHADERS);
	if (pFile && pFile->m_bGood) {
		_MESSAGE("Loaded %s", apFilename);
		uint32_t uiSize = 0;

		uiSize = pFile->GetSize();
		pShaderData = new DWORD[uiSize];
		bFreeMem = true;

		pFile->ReadBuffer(pShaderData, uiSize);
		delete pFile;
	}

	if (!pShaderData) {
		ShaderBuffer* pShaderPkg = BSShaderManager::GetShaderBuffer();
		if (pShaderPkg) {
			ShaderBuffer::RawShader* pRaw = pShaderPkg->GetShader(apFilename);
			if (pRaw)
				pShaderData = &pRaw->shader;
		}
		else {
			pShaderPkg = BSShaderManager::CreateShaderBuffer();
			pShaderPkg->Load(pShaderPkg->pPackagePath);
			ShaderBuffer::RawShader* pRaw = pShaderPkg->GetShader(apFilename);
			if (pRaw)
				pShaderData = &pRaw->shader;
			bLoadedPackageOnDemand = true;
		}
	}

	if (!pShaderData) {
		if (bFreeMem)
			delete[] pShaderData;

		_MESSAGE("Failed to find %s", apFilename);
		return nullptr;
	}

	NiDX9Renderer* pRenderer = BSShaderManager::GetRenderer();
	void* pNiShader = aFunc(pShaderData);

	if (bFreeMem)
		delete[] pShaderData;

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
	SL_ShaderRefresh = 0, // Sent on RefreshShaders
	SL_IS_PreRender = 1, // Sent pre rendering of EOF image space effects
	SL_IS_PostRender = 2, // Sent after rendering of EOF image space effects
};

static void __cdecl BSShaderManager__ReloadShaders() {
	ShaderBuffer* pBuffer = BSShaderManager::CreateShaderBuffer();
	pBuffer->Load(pBuffer->pPackagePath);
	CdeclCall(bGECK ? 0x8FE4A0 : 0xB557D0);
	if (pNVSEMessaging) {
		_MESSAGE("Reloading shaders");
		pNVSEMessaging->Dispatch(uiPluginHandle, SL_ShaderRefresh, nullptr, 0, nullptr);
	}
	BSShaderManager::DestroyShaderBuffer();
}

static void CleanupShaderBuffer() {
	if (bLoadedPackageOnDemand)
		uiShaderCleanupFrameCounter++;

	if (uiShaderCleanupFrameCounter > 360) {
		BSShaderManager::DestroyShaderBuffer();
		bLoadedPackageOnDemand = false;
		uiShaderCleanupFrameCounter = 0;
	}
}

class ImageSpaceManagerEx : public ImageSpaceManager {
public:
	void RenderEndOfFrameEffects(NiDX9Renderer* apRenderer, BSRenderedTexture*& apSourceBuffer, BSRenderedTexture*& apDestinationBuffer) {
		if (pNVSEMessaging) {
			pNVSEMessaging->Dispatch(uiPluginHandle, SL_IS_PreRender, nullptr, 0, nullptr);
		}

		std::vector<ImageSpaceEffect*> kActiveEffects;

		for (int i = IS_EFFECT_BLOOM; i < IS_EFFECT_VOLUMETRIC_FOG; i++) {
			for (int j = 0; j < kAdditionalEOFEffects.size(); j++) {
				auto kAdditionalEffect = kAdditionalEOFEffects.at(j);

				if (kAdditionalEffect.uiIndex >= (i + 1) * uiEOFEffectIndexDivider)
					break;

				if (kAdditionalEffect.pEffect->IsActive())
					kActiveEffects.push_back(kAdditionalEffect.pEffect);
			}

			ImageSpaceEffect* pEffect = GetEffect(i);

			if (pEffect && pEffect->IsActive())
				kActiveEffects.push_back(pEffect);
		}

		pEOFDest = apDestinationBuffer;

		if (kActiveEffects.empty()) [[unlikely]] {
			RenderEffect(IS_SHADER_COPY, apRenderer, apSourceBuffer, apDestinationBuffer, nullptr, true);
		}
		else [[likely]] {
			BSRenderedTexture* pSource = apSourceBuffer;
			uint32_t uiCount = kActiveEffects.size();
			BSRenderedTexture* pTarget = nullptr;

			if (uiCount == 1) {
				pTarget = apDestinationBuffer;
			}
			else {
				pTarget = pSwapTarget;
			}

			for (int32_t i = 0; i < kActiveEffects.size(); ++i) {
				if (i == kActiveEffects.size() - 1)
					pTarget = apDestinationBuffer;

				ImageSpaceEffect* pISEffect = kActiveEffects.at(i);
				RenderEffect(pISEffect, apRenderer, pSource, pTarget, nullptr, true);
				BSRenderedTexture* pTemp = pSource;
				pSource = pTarget;
				pTarget = pTemp;
			}
		}

		if (pNVSEMessaging) {
			pNVSEMessaging->Dispatch(uiPluginHandle, SL_IS_PostRender, nullptr, 0, nullptr);
		}
	}
};

static void GECKMessageHandler(NVSEMessagingInterface::Message* apMessage) {
	switch(apMessage->type){
	case NVSEMessagingInterface::kMessage_PostPostLoad:
		// Undo extender's shader buffer destruction patch
		SafeWrite8(0x9001F8, 0x75);
		SafeWrite8(0x9002AA, 0x75);
		break;
	}
}

static void GameMessageHandler(NVSEMessagingInterface::Message* apMessage) {
	switch (apMessage->type) {
	case NVSEMessagingInterface::kMessage_MainGameLoop:
		CleanupShaderBuffer();
		break;
	}
}

EXTERN_DLL_EXPORT NiD3DPixelShader* __cdecl CreatePixelShader(const char* apFilename) {
	return BSShader::CreatePixelShaderEx(nullptr, nullptr, nullptr, nullptr, nullptr, apFilename);
}

EXTERN_DLL_EXPORT NiD3DVertexShader* __cdecl CreateVertexShader(const char* apFilename) {
	return BSShader::CreateVertexShaderEx(nullptr, nullptr, nullptr, nullptr, nullptr, apFilename);
}

EXTERN_DLL_EXPORT void __cdecl RegisterEOFEffect(uint32_t auiIndex, ImageSpaceEffect* apEffect) {
	if (!apEffect)
		return;

	_MESSAGE("Register custom EOF effect at index %d", auiIndex);

	EOFISEffect kEffect{ auiIndex, apEffect };

	kAdditionalEOFEffects.insert(
		std::upper_bound(kAdditionalEOFEffects.begin(), kAdditionalEOFEffects.end(), kEffect, EffectOrder),
		kEffect
	);
}

EXTERN_DLL_EXPORT bool NVSEPlugin_Query(const NVSEInterface* nvse, PluginInfo* info) {
	info->infoVersion = PluginInfo::kInfoVersion;
	info->name = "Shader Loader";
	info->version = 140;

#if SUPPORT_GECK
	if (nvse)
		bGECK = nvse->isEditor;
#else
	if (nvse)
		return !nvse->isEditor;
#endif
	return true;
}

EXTERN_DLL_EXPORT bool NVSEPlugin_Load(NVSEInterface* nvse) {
	_MESSAGE("Initializing in %s", bGECK ? "GECK" : "Game");

	pNVSEMessaging = (NVSEMessagingInterface*)nvse->QueryInterface(kInterface_Messaging);
	uiPluginHandle = nvse->GetPluginHandle();

	if (bGECK) {
		WriteRelJump(0x975910, BSShader::CreateVertexShaderEx);
		WriteRelJump(0x976080, BSShader::CreatePixelShaderEx);
		ReplaceCall(0x44E92F, BSShaderManager__ReloadShaders);

		pNVSEMessaging->RegisterListener(uiPluginHandle, "NVSE", GECKMessageHandler);
	}
	else {
		bNVRPresent = GetModuleHandle("NewVegasReloaded.dll") != nullptr;

		WriteRelJump(0xBE0FE0, BSShader::CreateVertexShaderEx);
		WriteRelJump(0xBE1750, BSShader::CreatePixelShaderEx);

		for (uint32_t uiAddr : {0x5BF43C, 0x5C5A39})
			ReplaceCall(uiAddr, BSShaderManager__ReloadShaders);

		WriteRelJumpEx(0xB97900, &ImageSpaceManagerEx::RenderEndOfFrameEffects);

		pNVSEMessaging->RegisterListener(uiPluginHandle, "NVSE", GameMessageHandler);
	}

	return true;
}

BOOL WINAPI DllMain(
	HANDLE  hDllHandle,
	DWORD   dwReason,
	LPVOID  lpreserved
)
{
	return TRUE;
}
