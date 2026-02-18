#include "GameData.hpp"
#include "Shared/BSMemory/BSScrapMemory.hpp"
#include "nvse/PluginAPI.h"
#include "shared/Utils/DebugLog.hpp"

#include <algorithm>
#include <vector>

BS_ALLOCATORS;

constexpr const char* PLUGIN_NAME = "Shader Loader";
constexpr uint32_t PLUGIN_VERSION = 140;

IDebugLog	   gLog("logs\\ShaderLoader.log");

static NVSEMessagingInterface*			pNVSEMessaging = nullptr;
static PluginHandle						uiPluginHandle = 0;

namespace ShaderLoader {
	
	static bool								bLoadedPackageOnDemand = false;
	static uint32_t							uiShaderCleanupFrameCounter = 0;
	static bool								bNVRPresent = false;

	static constexpr uint32_t				EOF_EFFECT_INDEX_DIVIDER = 1000u;

	struct EOFISEffect {
		uint32_t			uiIndex;
		ImageSpaceEffect* pEffect;
	};
	static std::vector<EOFISEffect>			kAdditionalEOFEffects;

	struct ImageSpaceMessage {
		BSRenderedTexture* pSource;
		BSRenderedTexture* pDestination;
	};

	enum ShaderLoaderMessages {
		SL_ShaderRefresh = 0, // Sent on RefreshShaders
		SL_IS_PreRender  = 1, // Sent pre rendering of EOF image space effects
		SL_IS_PostRender = 2, // Sent after rendering of EOF image space effects
	};

	static void CleanupShaderBuffer() {
		if (bLoadedPackageOnDemand)
			uiShaderCleanupFrameCounter++;

		if (uiShaderCleanupFrameCounter > 360) {
			BSShaderManager::DestroyShaderBuffer();
			bLoadedPackageOnDemand = false;
			uiShaderCleanupFrameCounter = 0;
		}
	}

	static bool EffectOrder(const EOFISEffect& arEffect1, const EOFISEffect& arEffect2) {
		return arEffect1.uiIndex < arEffect2.uiIndex;
	}

	template <typename SHADER_TYPE>
	void* __fastcall CompileShader(const char* apFilename, DWORD* apShaderData) {
		using D3D_SHADER = std::conditional_t<std::is_same_v<SHADER_TYPE, NiD3DVertexShader>, LPDIRECT3DVERTEXSHADER9, LPDIRECT3DPIXELSHADER9>;
		SHADER_TYPE* pNiShader = nullptr;
		D3D_SHADER pShader = nullptr;
		NiDX9Renderer* pRenderer = BSShaderManager::GetRenderer();
		bool bShaderCreated = false;
		if constexpr (std::is_nothrow_convertible_v<SHADER_TYPE, NiD3DVertexShader>)
			bShaderCreated = SUCCEEDED(pRenderer->GetD3DDevice()->CreateVertexShader(apShaderData, &pShader));
		else if constexpr (std::is_nothrow_convertible_v<SHADER_TYPE, NiD3DPixelShader>)
			bShaderCreated = SUCCEEDED(pRenderer->GetD3DDevice()->CreatePixelShader(apShaderData, &pShader));

		if (bShaderCreated) {
			pNiShader = SHADER_TYPE::Create(pRenderer);
			pNiShader->SetShaderHandle(pShader);
			pNiShader->SetName(apFilename);

			if (bNVRPresent) {
				pNiShader->bEnabled = true;
				pNiShader->pShaderHandleBackup = pShader;
			}
		}
		return pNiShader;
	}

	
	__declspec(noinline) void* __fastcall CreateShader(const char* apFilename, void*(__fastcall* apShaderCompileFunc)(const char*, DWORD*)) {
		DWORD* pShaderData = nullptr;
		bool bFreeMem = false;
		char cPath[MAX_PATH];
		sprintf_s(cPath, "Data\\Shaders\\Loose\\%s", apFilename);
		BSFile* pFile = FileFinder::GetFile(cPath, NiFile::READ_ONLY, B_KiB(8), ARCHIVE_TYPE_SHADERS);
		if (pFile && pFile->m_bGood) {
			_MESSAGE("Loaded %s", apFilename);
			uint32_t uiSize = 0;

			uiSize = pFile->GetSize();
			pShaderData = BSScrapMemory::malloc<DWORD>(uiSize);
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
			_MESSAGE("Failed to find %s", apFilename);
			return nullptr;
		}

		void* pNiShader = apShaderCompileFunc(apFilename, pShaderData);

		if (bFreeMem)
			BSScrapMemory::free(pShaderData);

		return pNiShader;
	}

	static void GECKMessageHandler(NVSEMessagingInterface::Message* apMessage) {
		switch (apMessage->type) {
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
}

namespace Hooks {
	class BSShaderManagerEx : public BSShaderManager {
	public:
		static void ReloadShaders() {
			ShaderBuffer* pBuffer = CreateShaderBuffer();
			pBuffer->Load(pBuffer->pPackagePath);
			CdeclCall(bGECK ? 0x8FE4A0 : 0xB557D0);
			if (pNVSEMessaging) {
				_MESSAGE("Reloading shaders");
				pNVSEMessaging->Dispatch(uiPluginHandle, ShaderLoader::SL_ShaderRefresh, nullptr, 0, nullptr);
			}
			DestroyShaderBuffer();
		}
	};

	class BSShaderEx : public BSShader {
	public:
		// 0xBE0FE0
		static NiD3DVertexShader* __fastcall CreateVertexShader(BSShader* apThis, void*, const char* apPath, D3DXMACRO* apMacro, const char* apShaderVersion, const char* apFilename) {
			return static_cast<NiD3DVertexShader*>(ShaderLoader::CreateShader(apFilename, ShaderLoader::CompileShader<NiD3DVertexShader>));
		}

		// 0xBE1750
		static NiD3DPixelShader* __fastcall CreatePixelShader(BSShader* apThis, void*, const char* apPath, D3DXMACRO* apMacro, const char* apShaderVersion, const char* apFilename) {
			return static_cast<NiD3DPixelShader*>(ShaderLoader::CreateShader(apFilename, ShaderLoader::CompileShader<NiD3DPixelShader>));
		}
	};

	class ImageSpaceManagerEx : public ImageSpaceManager {
	public:
		void RenderEndOfFrameEffects(NiDX9Renderer* apRenderer, BSRenderedTexture*& apSourceBuffer, BSRenderedTexture*& apDestinationBuffer) {
			ShaderLoader::ImageSpaceMessage kMessage{ apSourceBuffer, apDestinationBuffer };
			pNVSEMessaging->Dispatch(uiPluginHandle, ShaderLoader::SL_IS_PreRender, &kMessage, sizeof(kMessage), nullptr);

			std::vector<ImageSpaceEffect*, BSScrapAllocator<ImageSpaceEffect*>> kActiveEffects(16);
			{
				eLastEffect = -1;
				iActiveEffectsCount = 0;
				for (uint32_t i = IS_EFFECT_BLOOM; i < IS_EFFECT_VOLUMETRIC_FOG; i++) {
					for (uint32_t j = 0; j < ShaderLoader::kAdditionalEOFEffects.size(); j++) {
						auto& rAdditionalEffect = ShaderLoader::kAdditionalEOFEffects.at(j);

						if (rAdditionalEffect.uiIndex >= (i + 1) * ShaderLoader::EOF_EFFECT_INDEX_DIVIDER)
							break;

						if (rAdditionalEffect.pEffect && rAdditionalEffect.pEffect->IsActive())
							kActiveEffects.push_back(rAdditionalEffect.pEffect);
					}

					ImageSpaceEffect* pEffect = GetEffect(i);

					if (pEffect && pEffect->IsActive()) {
						kActiveEffects.push_back(pEffect);
						eLastEffect = i;
					}
				}

				iActiveEffectsCount = kActiveEffects.size();
			}

			pEOFDest = apDestinationBuffer;

			if (kActiveEffects.empty()) [[unlikely]] {
				RenderEffect(IS_SHADER_COPY, apRenderer, apSourceBuffer, apDestinationBuffer, nullptr, true);
			}
			else [[likely]] {
				const uint32_t uiCount = kActiveEffects.size();
				BSRenderedTexture* pSource = apSourceBuffer;
				BSRenderedTexture* pTarget = nullptr;
				BSRenderedTexture* pSwapRT = nullptr;

				if (uiCount == 1) [[unlikely]]
					pTarget = apDestinationBuffer;
				else {
					pTarget = pSwapTarget;
					if (uiCount > 2)
						pSwapRT = BSShaderManager::GetTextureManager()->BorrowRenderedTexture(apRenderer, 3, 0, nullptr, 0);
				}

				uint32_t uiRenderedEffects = 0;
				for (uint32_t i = 0; i <= uiCount; i++) {
					if (uiRenderedEffects == 1 && pSwapRT)
						pTarget = pSwapRT;
					
					if (i == uiCount)
						pTarget = apDestinationBuffer;

					ImageSpaceEffect* pISEffect = kActiveEffects.at(i);
					if (pISEffect && pISEffect->IsActive()) {
						RenderEffect(pISEffect, apRenderer, pSource, pTarget, nullptr, true);
						++uiRenderedEffects;
						BSRenderedTexture* pTemp = pSource;
						pSource = pTarget;
						pTarget = pTemp;
					}
				}

				if (pSwapRT)
					BSShaderManager::GetTextureManager()->ReturnRenderedTexture(pSwapRT);
			}

			pNVSEMessaging->Dispatch(uiPluginHandle, ShaderLoader::SL_IS_PostRender, &kMessage, sizeof(kMessage), nullptr);
		}
	};
}

EXTERN_DLL_EXPORT NiD3DPixelShader* __cdecl CreatePixelShader(const char* apFilename) {
	return Hooks::BSShaderEx::CreatePixelShader(nullptr, nullptr, nullptr, nullptr, nullptr, apFilename);
}

EXTERN_DLL_EXPORT NiD3DVertexShader* __cdecl CreateVertexShader(const char* apFilename) {
	return Hooks::BSShaderEx::CreateVertexShader(nullptr, nullptr, nullptr, nullptr, nullptr, apFilename);
}

EXTERN_DLL_EXPORT void __cdecl RegisterEOFEffect(uint32_t auiIndex, ImageSpaceEffect* apEffect) {
	if (!apEffect)
		return;

	_MESSAGE("Register custom EOF effect at index %d", auiIndex);

	ShaderLoader::EOFISEffect kEffect{ auiIndex, apEffect };

	ShaderLoader::kAdditionalEOFEffects.insert(
		std::upper_bound(ShaderLoader::kAdditionalEOFEffects.begin(), ShaderLoader::kAdditionalEOFEffects.end(), kEffect, ShaderLoader::EffectOrder),
		kEffect
	);
}

EXTERN_DLL_EXPORT bool NVSEPlugin_Query(const NVSEInterface* nvse, PluginInfo* info) {
	info->infoVersion	= PluginInfo::kInfoVersion;
	info->name			= PLUGIN_NAME;
	info->version		= PLUGIN_VERSION;

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

	pNVSEMessaging = reinterpret_cast<NVSEMessagingInterface*>(nvse->QueryInterface(kInterface_Messaging));
	uiPluginHandle = nvse->GetPluginHandle();

	if (bGECK) {
		WriteRelJump(0x975910, Hooks::BSShaderEx::CreateVertexShader);
		WriteRelJump(0x976080, Hooks::BSShaderEx::CreatePixelShader);

		ReplaceCall(0x44E92F, Hooks::BSShaderManagerEx::ReloadShaders);

		WriteRelJumpEx(0x91EFF0, &Hooks::ImageSpaceManagerEx::RenderEndOfFrameEffects);

		pNVSEMessaging->RegisterListener(uiPluginHandle, "NVSE", ShaderLoader::GECKMessageHandler);
	}
	else {
		ShaderLoader::bNVRPresent = GetModuleHandle("NewVegasReloaded.dll") != nullptr;

		WriteRelJump(0xBE0FE0, Hooks::BSShaderEx::CreateVertexShader);
		WriteRelJump(0xBE1750, Hooks::BSShaderEx::CreatePixelShader);

		for (uint32_t uiAddr : {0x5BF43C, 0x5C5A39})
			ReplaceCall(uiAddr, Hooks::BSShaderManagerEx::ReloadShaders);

		WriteRelJumpEx(0xB97900, &Hooks::ImageSpaceManagerEx::RenderEndOfFrameEffects);
		
		pNVSEMessaging->RegisterListener(uiPluginHandle, "NVSE", ShaderLoader::GameMessageHandler);
	}

	return true;
}

BOOL WINAPI DllMain(
	HANDLE  hDllHandle,
	DWORD   dwReason,
	LPVOID  lpreserved
)
{
	if (dwReason == DLL_PROCESS_ATTACH)
		DisableThreadLibraryCalls(static_cast<HMODULE>(hDllHandle));
	return TRUE;
}
