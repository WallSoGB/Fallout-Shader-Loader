#pragma once

#include "SafeWrite.h"
#include "Utilities.h"

#define ASSERT_SIZE(a, b) static_assert(sizeof(a) == b, "Wrong structure size!");
#define ASSERT_OFFSET(a, b, c) static_assert(offsetof(a, b) == c, "Wrong member offset!");
#define CREATE_OBJECT(CLASS, ADDRESS) static CLASS* CreateObject() { return StdCall<CLASS*>(ADDRESS); };
#ifdef FO3
#define IS_NODE(object) ((*(UInt32**)object)[3 * 4 >> 2] == 0xAA2340)
#else
#define IS_NODE(object) ((*(UInt32**)object)[3 * 4 >> 2] == 0x6815C0)
#endif

class BGSDistantObjectBlock;
class bhkBlendCollisionObject;
class bhkCollisionObject;
class bhkLimitedHingeConstraint;
class bhkRigidBody;
class BSFadeNode;
class BSMultiBoundNode;
class BSMultiBound;
class BSResizableTriShape;
class BSSegmentedTriShape;
class NiCloningProcess;
class NiGeometry;
class NiLines;
class NiNode;
class NiParticles;
class NiStream;
class NiTriBasedGeom;
class NiTriShape;
class NiTriStrips;
class NiControllerManager;
class NiObjectGroup;
class NiObjectNET;
class NiBound;
class NiViewerStringsArray;
class NiUpdateData;
class NiMatrix3;
class NiCullingProcess;
class NiFixedString;
class NiGeometryData;
class NiSkinInstance;
class bhkNiCollisionObject;
class NiDX9Renderer;
class BSShader;
class BGSTextureUseMap;
class NiSourceTexture;
class RenderPassArray;
class RenderPass;
class BSShaderAccumulator;
class NiAdditionalGeometryData;
class NiGeometryBufferData;
class NiD3DPass;
class NiD3DShaderDeclaration;
class NiD3DRenderStateGroup;
class NiD3DShaderConstantMap;
class NiDX9ShaderDeclaration;
class NiDynamicEffectState;
class BGSDistantTreeBlock;
class BGSTerrainChunk;
class NiProperty;
class NiNode;
class BSMultiBoundRoom;
class NiTimeController;
class BSString;
class NiD3DShaderProgramCreator;
class NiShaderConstantMapEntry;
class NiD3DRenderState;

static void* NiNew(size_t stSize) {
	return CdeclCall<void*>(0xAA13E0, stSize);
}

template <typename T_Data>
static T_Data* NiNew() {
	return (T_Data*)NiNew(sizeof(T_Data));
}

template <typename T, const UInt32 ConstructorPtr = 0, typename... Args>
T* NiCreate(Args &&... args) {
	auto* alloc = NiNew(sizeof(T));
	if constexpr (ConstructorPtr) {
		ThisStdCall(ConstructorPtr, alloc, std::forward<Args>(args)...);
	}
	else {
		memset(alloc, 0, sizeof(T));
	}
	return static_cast<T*>(alloc);
}

class NiMatrix3 {
public:
	float m_pEntry[3][3];
};

class NiPoint2 {
public:
	float x, y;
};

class NiPoint3 {
public:
	float x, y, z;

	NiPoint3 operator- (const NiPoint3& arPt) const { return NiPoint3(x - arPt.x, y - arPt.y, z - arPt.z); };
};

class NiPoint4 {
public:
	float x, y, z, w;
};

class NiTransform {
public:
	NiMatrix3	m_Rotate;
	NiPoint3	m_Translate;
	float		m_fScale;
};

class NiRTTI {
public:
	const char*		m_pcName;
	const NiRTTI*	m_pkBaseRTTI;
};

template <typename T_Data>
class NiTArray {
public:
	virtual ~NiTArray();

	T_Data* m_pBase;
	UInt16 m_usMaxSize;
	UInt16 m_usSize;
	UInt16 m_usESize;
	UInt16 m_usGrowBy;
};

ASSERT_SIZE(NiTArray<void*>, 0x10);

typedef void* NiTListIterator;

template <typename T_Data>
class NiTListItem {
public:
	NiTListItem*	m_pkNext;
	NiTListItem*	m_pkPrev;
	T_Data			m_element;
};

template <typename T_Data>
class NiTListBase {
public:
	NiTListItem<T_Data>*	m_pkHead;
	NiTListItem<T_Data>*	m_pkTail;
	UInt32					m_uiCount;

	inline UInt32 GetSize() const { return m_uiCount; };
	bool IsEmpty() const { return m_uiCount == 0; };

	NiTListIterator GetHeadPos() const { return m_pkHead; };
	NiTListIterator GetTailPos() const { return m_pkTail; };
};


template <class T_Data>
class NiPointer {
public:
	NiPointer(T_Data* apObject = (T_Data*)0) {
		m_pObject = apObject;
		if (m_pObject)
			m_pObject->IncRefCount();
	}
	NiPointer(const NiPointer& arPointer) {
		m_pObject = arPointer.m_pObject;
		if (m_pObject)
			m_pObject->IncRefCount();
	}
	~NiPointer() {
		if (m_pObject)
			m_pObject->DecRefCount();
	}

	T_Data* m_pObject;

	__forceinline NiPointer<T_Data>& operator =(const NiPointer& arPointer) {
		if (m_pObject != arPointer.m_pObject) {
			if (m_pObject)
				m_pObject->DecRefCount();
			m_pObject = arPointer.m_pObject;
			if (m_pObject)
				m_pObject->IncRefCount();
		}
		return *this;
	}

	__forceinline NiPointer<T_Data>& operator =(T_Data* apObject) {
		if (m_pObject != apObject) {
			if (m_pObject)
				m_pObject->DecRefCount();
			m_pObject = apObject;
			if (m_pObject)
				m_pObject->IncRefCount();
		}
		return *this;
	}

	__forceinline bool operator==(T_Data* apObject) const { return (m_pObject == apObject); }
	__forceinline bool operator==(const NiPointer& arPointer) const { return (m_pObject == arPointer.m_pObject); }
	__forceinline operator bool() const { return m_pObject != nullptr; }
	__forceinline operator T_Data* () const { return m_pObject; }
	__forceinline T_Data& operator*() const { return *m_pObject; }
	__forceinline T_Data* operator->() const { return m_pObject; }
};

class NiRefObject {
public:
    virtual			~NiRefObject();
    virtual void	DeleteThis();

    UInt32 m_uiRefCount;

    // 0x40F6E0
    inline void IncRefCount() {
        InterlockedIncrement(&m_uiRefCount);
    }

    // 0x401970
    inline void DecRefCount() {
        if (!InterlockedDecrement(&m_uiRefCount))
            DeleteThis();
    }
};

class NiObject : public NiRefObject {
public:
    virtual const NiRTTI*				GetRTTI() const;												// 02 | Returns NiRTTI of the object
	virtual NiNode*						IsNiNode() const;												// 03 | Returns this if it's a NiNode, otherwise nullptr
	virtual BSFadeNode*					IsFadeNode() const;												// 04 | Returns this if it's a BSFadeNode, otherwise nullptr
	virtual BSMultiBoundNode*			IsMultiBoundNode() const;										// 05 | Returns this if it's a BSMultiBoundNode, otherwise nullptr
	virtual NiGeometry*					IsGeometry() const;												// 06 | Returns this if it's a NiGeometry, otherwise nullptr
	virtual NiTriBasedGeom*				IsTriBasedGeometry() const;										// 07 | Returns this if it's a NiTriBasedGeom, otherwise nullptr
	virtual NiTriStrips*				IsTriStrips() const;											// 08 | Returns this if it's a NiTriStrips, otherwise nullptr
	virtual NiTriShape*					IsTriShape() const;												// 09 | Returns this if it's a NiTriShape, otherwise nullptr
	virtual BSSegmentedTriShape*		IsSegmentedTriShape() const;									// 10 | Returns this if it's a BSSegmentedTriShape, otherwise nullptr
	virtual BSResizableTriShape*		IsResizableTriShape() const;									// 11 | Returns this if it's a BSResizableTriShape, otherwise nullptr
	virtual NiParticles*				IsParticlesGeom() const;										// 12 | Returns this if it's a NiParticles, otherwise nullptr
	virtual NiLines*					IsLinesGeom() const;											// 13 | Returns this if it's a NiLines, otherwise nullptr
	virtual bhkCollisionObject*			IsBhkNiCollisionObject() const;									// 14 | Returns this if it's a bhkCollisionObject, otherwise nullptr
	virtual bhkBlendCollisionObject*	IsBhkBlendCollisionObject() const;								// 15 | Returns this if it's a bhkBlendCollisionObject, otherwise nullptr
	virtual bhkRigidBody*				IsBhkRigidBody() const;											// 16 | Returns this if it's a bhkRigidBody, otherwise nullptr
	virtual bhkLimitedHingeConstraint*	IsBhkLimitedHingeConstraint() const;							// 17 | Returns this if it's a bhkLimitedHingeConstraint, otherwise nullptr
	virtual NiObject*					CreateClone(NiCloningProcess* apCloning);						// 18 | Creates a clone of this object
	virtual void						LoadBinary(NiStream* apStream);									// 19 | Loads objects from disk
	virtual void						LinkObject(NiStream* apStream);									// 20 | Called by the streaming system to resolve links to other objects once it can be guaranteed that all objects have been loaded
	virtual void						RegisterStreamables(NiStream* apStream);						// 21 | When an object is inserted into a stream, it calls register streamables to make sure that any contained objects or objects linked in a scene graph are streamed as well
	virtual void						SaveBinary(NiStream* apStream);									// 22 | Saves objects to disk
	virtual bool						IsEqual(NiObject* apObject) const;								// 23 | Compares this object with another
	virtual void						GetViewerStrings(NiViewerStringsArray* apStrings);				// 24 | Gets strings containing information about the object
	virtual void						AddViewerStrings(NiViewerStringsArray* apStrings);				// 25 | Adds additional strings containing information about the object
	virtual void						ProcessClone(NiCloningProcess* apCloning);						// 26 | Post process for CreateClone
	virtual void						PostLinkObject(NiStream* apStream);								// 27 | Called by the streaming system to resolve any tasks that require other objects to be correctly linked. It is called by the streaming system after LinkObject has been called on all streamed objects
	virtual bool						StreamCanSkip();												// 28
	virtual const NiRTTI*				GetStreamableRTTI();											// 29
	virtual void						SetBound(NiBound* apNewBound);									// 30 | Replaces the bound of the object
	virtual void						GetBlockAllocationSize();										// 31 | Used by geometry data
	virtual NiObjectGroup*				GetGroup();														// 32 | Used by geometry data
	virtual void						SetGroup(NiObjectGroup* apGroup);								// 33 | Used by geometry data
	virtual NiControllerManager*		IsControllerManager() const;									// 34 | Returns this if it's a NiControllerManager, otherwise nullptr

	// 0x6532C0
	bool IsKindOf(const NiRTTI& apRTTI) const {
		for (const NiRTTI* i = GetRTTI(); i; i = i->m_pkBaseRTTI) {
			if (i == &apRTTI)
				return true;
		}
		return false;
	}
};
class NiObjectNET : public NiObject {
public:
	const char*						m_kName;
	NiPointer<NiTimeController>		m_spControllers;
	void**							m_ppkExtra;
	UInt16							m_usExtraDataSize;
	UInt16							m_usMaxSize;
};

class NiAVObject : public NiObjectNET {
public:
	virtual void			UpdateControllers(NiUpdateData& arData);
	virtual void			ApplyTransform(NiMatrix3& akMat, NiPoint3& akTrn, bool abOnLeft);
	virtual void			Unk_39();
	virtual NiAVObject*		GetObject_(const NiFixedString& kName);
	virtual NiAVObject*		GetObjectByName(const NiFixedString& kName);
	virtual void			SetSelectiveUpdateFlags(bool* bSelectiveUpdate, bool bSelectiveUpdateTransforms, bool* bRigid);
	virtual void			UpdateDownwardPass(const NiUpdateData& arData, UInt32 uFlags);
	virtual void			UpdateSelectedDownwardPass(const NiUpdateData& arData, UInt32 uFlags);
	virtual void			UpdateRigidDownwardPass(const NiUpdateData& arData, UInt32 uFlags);
	virtual void			Unk_46(void* arg);
	virtual void			UpdateTransform();
	virtual void			UpdateWorldData(const NiUpdateData& arData);
	virtual void			UpdateWorldBound();
	virtual void			UpdateTransformAndBounds(const NiUpdateData& arData);
	virtual void			PreAttachUpdate(NiNode* pEventualParent, const NiUpdateData& arData);
	virtual void			PreAttachUpdateProperties(NiNode* pEventualParent);
	virtual void			DetachParent();
	virtual void			UpdateUpwardPassParent(void* arg);
	virtual void			OnVisible(NiCullingProcess* kCuller);
	virtual void			PurgeRendererData(NiDX9Renderer* apRenderer);

	NiNode*							m_pkParent;
	bhkNiCollisionObject*			m_spCollisionObject;
	NiBound*						m_kWorldBound;
	NiTListBase<NiProperty*>		m_kPropertyList;
	Bitfield32						m_uiFlags;
	NiTransform						m_kLocal;
	NiTransform						m_kWorld;
};

class NiNode : public NiAVObject {
public:
	virtual void			AttachChild(NiAVObject* apChild, bool abFirstAvail);
	virtual void			InsertChildAt(UInt32 i, NiAVObject* apChild);
	virtual void			DetachChild(NiAVObject* apChild, NiAVObject*& aspAVObject);
	virtual void			DetachChildAlt(NiAVObject* apChild);
	virtual void			DetachChildAt(UInt32 i, NiAVObject*& aspAVObject);
	virtual NiAVObject*		DetachChildAtAlt(UInt32 i);
	virtual void			SetAt(UInt32 i, NiAVObject* apChild, NiAVObject*& aspAVObject);
	virtual void			SetAtAlt(UInt32 i, NiAVObject* apChild);
	virtual void			UpdateUpwardPass();

	NiTArray<NiAVObject*> m_kChildren;

    UInt32 GetChildCount() const {
		return m_kChildren.m_usESize;
    }

	UInt32 GetArrayCount() const {
		return m_kChildren.m_usSize;
	}

	NiAVObject* GetAt(UInt32 index) const {
		return m_kChildren.m_pBase[index];
	}
};

struct D3DXMACRO;

class NiGPUProgram : public NiRefObject {
public:
	NiGPUProgram();
	virtual ~NiGPUProgram();

	union {
		UInt32				m_eProgramType;

		struct {
			UInt8				ucProgramType;
			UInt8				empty[2];
			bool				bEnabled;
		}; // NVR
	};
};

class NiD3DShaderProgram : public NiGPUProgram {
public:
	NiD3DShaderProgram();
	virtual ~NiD3DShaderProgram();

	virtual NiRTTI*						GetRTTI();
	virtual NiNode*						IsNiNode();
	virtual BSFadeNode*					IsFadeNode();
	virtual BSMultiBoundNode*			IsMultiBoundNode();
	virtual NiGeometry*					IsGeometry();
	virtual NiTriBasedGeom*				IsTriBasedGeometry();
	virtual NiTriStrips*				IsTriStrips();
	virtual NiTriShape*					IsTriShape();
	virtual BSSegmentedTriShape*		IsSegmentedTriShape();
	virtual BSResizableTriShape*		IsResizableTriShape();
	virtual NiParticles*				IsParticlesGeom();
	virtual NiLines*					IsLinesGeom();
	virtual bhkNiCollisionObject*		IsBhkNiCollisionObject();
	virtual bhkBlendCollisionObject*	IsBhkBlendCollisionObject();
	virtual bhkRigidBody*				IsBhkRigidBody();
	virtual bhkLimitedHingeConstraint*	IsBhkLimitedHingeConstraint();
	virtual UInt32						GetVariableCount();
	virtual const char*					GetVariableName();
	virtual const char*					GetName();
	virtual const char*					SetName(const char* pszName);
	virtual char*						GetShaderProgramName();
	virtual void						SetShaderProgramName(const char* pszName);
	virtual UInt32						GetCodeSize();
	virtual void*						GetCode();
	virtual void						SetCode(UInt32 uiSize, void* pvCode);
	virtual NiD3DShaderProgramCreator*	GetCreator();
	virtual void						SetCreator(NiD3DShaderProgramCreator* pkCreator);
	virtual void						SetShaderConstant(NiShaderConstantMapEntry*, const void*, UInt32);
	virtual bool						SetShaderConstantArray(NiShaderConstantMapEntry* pkEntry, const void* pvDataSource, UInt32 uiNumEntries, UInt32 uiRegistersPerEntry, UInt16* pusReorderArray);

	char*						m_pszName;
	union {
		char*						m_pszShaderProgramName;
		UInt32						m_uiCodeSize;
		void*						m_pvCode;
		NiD3DShaderProgramCreator*	m_pkCreator;
		struct {
			void*		ShaderProg[3];
			IUnknown*	pShaderHandleBackup;
		}; // NVR
	};
	LPDIRECT3DDEVICE9			m_pkD3DDevice;
	NiDX9Renderer*				m_pkD3DRenderer;
	NiD3DRenderState*			m_pkD3DRenderState;
};

class NiD3DPixelShader : public NiD3DShaderProgram {
public:
	virtual LPDIRECT3DPIXELSHADER9	GetShaderHandle();
	virtual void					SetShaderHandle(LPDIRECT3DPIXELSHADER9 hShader);
	virtual void					DestroyRendererData(LPDIRECT3DPIXELSHADER9 hShader);
	virtual void					RecreateRendererData();

	LPDIRECT3DPIXELSHADER9 m_hShader;

	static NiD3DPixelShader* Create(NiDX9Renderer* apRenderer) {
		return NiCreate<NiD3DPixelShader, 0xBE08F0>(apRenderer);
	}
};

class NiD3DVertexShader : public NiD3DShaderProgram {
public:
	virtual UInt32						GetUsage();
	virtual void						SetUsage(UInt32 uiUsage);
	virtual LPDIRECT3DVERTEXSHADER9		GetShaderHandle();
	virtual void						SetShaderHandle(LPDIRECT3DVERTEXSHADER9 hShader);
	virtual LPDIRECT3DVERTEXSHADER9		GetVertexDeclaration();
	virtual void						SetVertexDeclaration(LPDIRECT3DVERTEXSHADER9 hShader);
	virtual bool						GetSoftwareVertexProcessing();
	virtual void						SetSoftwareVertexProcessing(bool bSoftwareVP);
	virtual void						DestroyRendererData();
	virtual void						RecreateRendererData();

	bool							m_bSoftwareVP;
	UInt32							m_uiUsage;
	LPDIRECT3DVERTEXSHADER9			m_hShader;
	LPDIRECT3DVERTEXDECLARATION9	m_hDecl;

	static NiD3DVertexShader* Create(NiDX9Renderer* apRenderer) {
		return NiCreate<NiD3DVertexShader, 0xBE0B30>(apRenderer);
	}
};

class BSShader {
public:
	static NiD3DVertexShader* __fastcall CreateVertexShaderEx(BSShader* apThis, void*, const char* apPath, D3DXMACRO* apMacro, const char* apShaderVersion, const char* apFilename);
	static NiD3DPixelShader* __fastcall CreatePixelShaderEx(BSShader* apThis, void*, const char* apPath, D3DXMACRO* apMacro, const char* apShaderVersion, const char* apFilename);
};

struct MemoryManager {
	static MemoryManager* GetSingleton() {
		return &*(MemoryManager*)0x11F6238;
	}

	template <typename T_Data>
	__declspec(restrict) __declspec(allocator) static T_Data* Allocate() {
		return static_cast<T_Data*>(Allocate(sizeof(T_Data)));
	};
	template <typename T_Data>
	__declspec(restrict) __declspec(allocator) static T_Data* Allocate(size_t aCount) {
		return static_cast<T_Data*>(Allocate(sizeof(T_Data) * aCount));
	};
	__declspec(restrict) __declspec(allocator) static void* Allocate(size_t aSize) {
		return ThisStdCall<void*>(0xAA3E40, MemoryManager::GetSingleton(), aSize);
	}
	static void Deallocate(void* apMemory) {
		ThisStdCall(0xAA4060, MemoryManager::GetSingleton(), apMemory);
	}
};

class NiBinaryStream {
public:
	NiBinaryStream();
	virtual ~NiBinaryStream();

	virtual 		operator bool();
	virtual void	Seek(SInt32 aiNumBytes);
	virtual UInt32	GetPosition() const;
	virtual void	SetEndianSwap(bool abDoSwap);

	UInt32	m_uiAbsoluteCurrentPos;
	void*	m_pfnRead;
	void*	m_pfnWrite;
};

class NiFile : public NiBinaryStream {
public:
	NiFile();
	virtual ~NiFile();

	virtual void		Seek(SInt32 aiOffset, SInt32 aiWhence);
	virtual const char* GetFilename() const;
	virtual UInt32		GetFileSize();

	enum OpenMode {
		READ_ONLY = 0x0,
		WRITE_ONLY = 0x1,
		APPEND_ONLY = 0x2,
	};

	UInt32		m_uiBufferAllocSize;
	UInt32		m_uiBufferReadSize;
	UInt32		m_uiPos;
	UInt32		m_uiAbsolutePos;
	char*		m_pBuffer;
	FILE*		m_pFile;
	OpenMode	m_eMode;
	bool		m_bGood;
};

class BSFile : public NiFile {
public:
	BSFile();
	virtual ~BSFile();

	virtual bool	Open(int = 0, bool abTextMode = false);
	virtual bool	OpenByFilePointer(FILE* apFile);
	virtual UInt32	GetSize();
	virtual UInt32	ReadString(BSString& arString, UInt32 auiMaxLength);
	virtual UInt32	ReadStringAlt(BSString& arString, UInt32 auiMaxLength);
	virtual UInt32	GetLine(char* apBuffer, UInt32 auiMaxBytes, UInt8 aucMark);
	virtual UInt32	WriteString(BSString& arString, bool abBinary);
	virtual UInt32	WriteStringAlt(BSString& arString, bool abBinary);
	virtual bool	IsReadable();
	virtual UInt32	DoRead(void* apBuffer, UInt32 auiBytes);
	virtual UInt32	DoWrite(const void* apBuffer, UInt32 auiBytes);

	UInt32 ReadBuffer(void* apData, UInt32 auiSize) {
		return ThisStdCall<UInt32>(0x462D80, this, apData, auiSize);
	}
};

enum ARCHIVE_TYPE {
	ARCHIVE_TYPE_ALL_		= 0xFFFFFFFF,
	ARCHIVE_TYPE_ALL		= 0xFFFF,
	ARCHIVE_TYPE_MESHES		= 0x1,
	ARCHIVE_TYPE_TEXTURES	= 0x2,
	ARCHIVE_TYPE_MENUS		= 0x4,
	ARCHIVE_TYPE_SOUNDS		= 0x8,
	ARCHIVE_TYPE_VOICES		= 0x10,
	ARCHIVE_TYPE_SHADERS	= 0x20,
	ARCHIVE_TYPE_TREES		= 0x40,
	ARCHIVE_TYPE_FONTS		= 0x80,
	ARCHIVE_TYPE_MISC		= 0x100,
	ARCHIVE_TYPE_COUNT		= 9,
};

struct FileFinder {
	enum LOOKIN_FLAGS : UInt8 {
		SKIP_NONE		= 0,
		SKIP_ARCHIVE	= 1,
		SKIP_CWD		= 2,
		SKIP_PATHS		= 4,
	};

	static FileFinder* GetSingleton() {
		return *(FileFinder**)0x11F81DC;
	}
	static BSFile* GetFile(const char* apName, NiFile::OpenMode aeMode, int aiSize, ARCHIVE_TYPE aeArchiveType) {
		return CdeclCall<BSFile*>(0xAFDF20, apName, aeMode, aiSize, aeArchiveType);
	}
};



class NiRenderer : public NiObject {
public:
	UInt32 padding[158];
};
ASSERT_SIZE(NiRenderer, 0x280);


class NiDX9Renderer : public NiRenderer {
public:
	LPDIRECT3DVERTEXDECLARATION9 hParticleVertexDecls[2];
	LPDIRECT3DDEVICE9			 m_pkD3DDevice9;

	LPDIRECT3DDEVICE9 GetD3DDevice() const {
		return m_pkD3DDevice9;
	}
};

class BSShaderManager {
public:
	struct ShaderPackage {
		struct RawShader {
			char	cFileName[260];
			DWORD	shader;
		};

		static ShaderPackage* GetSingleton() { return *(ShaderPackage**)0x11F9498; }
		RawShader* GetShader(const char* apcShaderName) {
			return ThisStdCall<RawShader*>(0xB7FAC0, this, apcShaderName);
		}
	};

	static NiDX9Renderer* GetRenderer() {
		return *(NiPointer<NiDX9Renderer>*)0x11F9508;
	}
};
