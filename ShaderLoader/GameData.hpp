#pragma once

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

	T_Data*  m_pBase;
	uint16_t m_usMaxSize;
	uint16_t m_usSize;
	uint16_t m_usESize;
	uint16_t m_usGrowBy;
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
	uint32_t				m_uiCount;

	inline uint32_t GetSize() const { return m_uiCount; };
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

    uint32_t m_uiRefCount;

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
};
class NiObjectNET : public NiObject {
public:
	const char*						m_kName;
	NiPointer<NiTimeController>		m_spControllers;
	void**							m_ppkExtra;
	uint16_t						m_usExtraDataSize;
	uint16_t						m_usMaxSize;
};

class NiAVObject : public NiObjectNET {
public:
	virtual void			UpdateControllers(NiUpdateData& arData);
	virtual void			ApplyTransform(NiMatrix3& akMat, NiPoint3& akTrn, bool abOnLeft);
	virtual void			Unk_39();
	virtual NiAVObject*		GetObject_(const NiFixedString& kName);
	virtual NiAVObject*		GetObjectByName(const NiFixedString& kName);
	virtual void			SetSelectiveUpdateFlags(bool* bSelectiveUpdate, bool bSelectiveUpdateTransforms, bool* bRigid);
	virtual void			UpdateDownwardPass(const NiUpdateData& arData, uint32_t uFlags);
	virtual void			UpdateSelectedDownwardPass(const NiUpdateData& arData, uint32_t uFlags);
	virtual void			UpdateRigidDownwardPass(const NiUpdateData& arData, uint32_t uFlags);
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
	virtual void			InsertChildAt(uint32_t i, NiAVObject* apChild);
	virtual void			DetachChild(NiAVObject* apChild, NiAVObject*& aspAVObject);
	virtual void			DetachChildAlt(NiAVObject* apChild);
	virtual void			DetachChildAt(uint32_t i, NiAVObject*& aspAVObject);
	virtual NiAVObject*		DetachChildAtAlt(uint32_t i);
	virtual void			SetAt(uint32_t i, NiAVObject* apChild, NiAVObject*& aspAVObject);
	virtual void			SetAtAlt(uint32_t i, NiAVObject* apChild);
	virtual void			UpdateUpwardPass();

	NiTArray<NiAVObject*> m_kChildren;

    uint32_t GetChildCount() const {
		return m_kChildren.m_usESize;
    }

	uint32_t GetArrayCount() const {
		return m_kChildren.m_usSize;
	}

	NiAVObject* GetAt(uint32_t index) const {
		return m_kChildren.m_pBase[index];
	}
};

struct D3DXMACRO;

class NiGPUProgram : public NiRefObject {
public:
	NiGPUProgram();
	virtual ~NiGPUProgram();

	union {
		uint32_t				m_eProgramType;

		struct {
			uint8_t				ucProgramType;
			uint8_t				empty[2];
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
	virtual uint32_t					GetVariableCount();
	virtual const char*					GetVariableName();
	virtual const char*					GetName();
	virtual const char*					SetName(const char* pszName);
	virtual char*						GetShaderProgramName();
	virtual void						SetShaderProgramName(const char* pszName);
	virtual uint32_t					GetCodeSize();
	virtual void*						GetCode();
	virtual void						SetCode(uint32_t uiSize, void* pvCode);
	virtual NiD3DShaderProgramCreator*	GetCreator();
	virtual void						SetCreator(NiD3DShaderProgramCreator* pkCreator);
	virtual void						SetShaderConstant(NiShaderConstantMapEntry*, const void*, uint32_t);
	virtual bool						SetShaderConstantArray(NiShaderConstantMapEntry* pkEntry, const void* pvDataSource, uint32_t uiNumEntries, uint32_t uiRegistersPerEntry, uint16_t* pusReorderArray);

	char*						m_pszName;
	union {
		char*						m_pszShaderProgramName;
		uint32_t					m_uiCodeSize;
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
		if (bGECK)
			return BSCreate<NiD3DPixelShader, 0x975260>(apRenderer);
		else
			return BSCreate<NiD3DPixelShader, 0xBE08F0>(apRenderer);
	}
};

class NiD3DVertexShader : public NiD3DShaderProgram {
public:
	virtual uint32_t					GetUsage();
	virtual void						SetUsage(uint32_t uiUsage);
	virtual LPDIRECT3DVERTEXSHADER9		GetShaderHandle();
	virtual void						SetShaderHandle(LPDIRECT3DVERTEXSHADER9 hShader);
	virtual LPDIRECT3DVERTEXSHADER9		GetVertexDeclaration();
	virtual void						SetVertexDeclaration(LPDIRECT3DVERTEXSHADER9 hShader);
	virtual bool						GetSoftwareVertexProcessing();
	virtual void						SetSoftwareVertexProcessing(bool bSoftwareVP);
	virtual void						DestroyRendererData();
	virtual void						RecreateRendererData();

	bool							m_bSoftwareVP;
	uint32_t						m_uiUsage;
	LPDIRECT3DVERTEXSHADER9			m_hShader;
	LPDIRECT3DVERTEXDECLARATION9	m_hDecl;

	static NiD3DVertexShader* Create(NiDX9Renderer* apRenderer) {
		if (bGECK)
			return BSCreate<NiD3DVertexShader, 0x975490>(apRenderer);
		else
			return BSCreate<NiD3DVertexShader, 0xBE0B30>(apRenderer);

	}
};

class BSShader {
public:
	static NiD3DVertexShader* __fastcall CreateVertexShaderEx(BSShader* apThis, void*, const char* apPath, D3DXMACRO* apMacro, const char* apShaderVersion, const char* apFilename);
	static NiD3DPixelShader* __fastcall CreatePixelShaderEx(BSShader* apThis, void*, const char* apPath, D3DXMACRO* apMacro, const char* apShaderVersion, const char* apFilename);
};

class NiBinaryStream {
public:
	NiBinaryStream();
	virtual ~NiBinaryStream();

	virtual 			operator bool();
	virtual void		Seek(int32_t aiNumBytes);
	virtual uint32_t	GetPosition() const;
	virtual void		SetEndianSwap(bool abDoSwap);

	uint32_t	m_uiAbsoluteCurrentPos;
	void*		m_pfnRead;
	void*		m_pfnWrite;
};

class NiFile : public NiBinaryStream {
public:
	NiFile();
	virtual ~NiFile();

	virtual void		Seek(int32_t aiOffset, int32_t aiWhence);
	virtual const char* GetFilename() const;
	virtual uint32_t	GetFileSize();

	enum OpenMode {
		READ_ONLY = 0x0,
		WRITE_ONLY = 0x1,
		APPEND_ONLY = 0x2,
	};

	uint32_t	m_uiBufferAllocSize;
	uint32_t	m_uiBufferReadSize;
	uint32_t	m_uiPos;
	uint32_t	m_uiAbsolutePos;
	char*		m_pBuffer;
	FILE*		m_pFile;
	OpenMode	m_eMode;
	bool		m_bGood;
};

class BSFile : public NiFile {
public:
	BSFile();
	virtual ~BSFile();

	virtual bool		Open(int = 0, bool abTextMode = false);
	virtual bool		OpenByFilePointer(FILE* apFile);
	virtual uint32_t	GetSize();
	virtual uint32_t	ReadString(BSString& arString, uint32_t auiMaxLength);
	virtual uint32_t	ReadStringAlt(BSString& arString, uint32_t auiMaxLength);
	virtual uint32_t	GetLine(char* apBuffer, uint32_t auiMaxBytes, uint8_t aucMark);
	virtual uint32_t	WriteString(BSString& arString, bool abBinary);
	virtual uint32_t	WriteStringAlt(BSString& arString, bool abBinary);
	virtual bool		IsReadable();
	virtual uint32_t	DoRead(void* apBuffer, uint32_t auiBytes);
	virtual uint32_t	DoWrite(const void* apBuffer, uint32_t auiBytes);

	uint32_t ReadBuffer(void* apData, uint32_t auiSize) {
		return ThisCall<uint32_t>(bGECK ? 0x401210 : 0x462D80, this, apData, auiSize);
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
	enum LOOKIN_FLAGS : uint8_t {
		SKIP_NONE		= 0,
		SKIP_ARCHIVE	= 1,
		SKIP_CWD		= 2,
		SKIP_PATHS		= 4,
	};

	static FileFinder* GetSingleton() {
		return *(FileFinder**)(bGECK ? 0xF22AA0 : 0x11F81DC);
	}

	static BSFile* GetFile(const char* apName, NiFile::OpenMode aeMode, int aiSize, ARCHIVE_TYPE aeArchiveType) {
		return CdeclCall<BSFile*>(bGECK ? 0x8A1E10 : 0xAFDF20, apName, aeMode, aiSize, aeArchiveType);

	}
};

class NiRenderer : public NiObject {
public:
	uint32_t padding[158];
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

class ShaderBuffer {
public:
	ShaderBuffer() {
		memset(this, 0, sizeof(ShaderBuffer));
		ThisCall(bGECK ? 0x958320 : 0xB7FF40, this, CdeclCall<uint32_t>(bGECK ? 0x957EF0 : 0xB7FAF0));
	}

	~ShaderBuffer() {
		ThisCall(bGECK ? 0x958110 : 0xB7FD50, this);
	}

	const char* pPackagePath;
	char content[0x18];

	struct RawShader {
		char	cFileName[260];
		DWORD	shader;
	};

	void Load(const char* apPath) {
		ThisCall(bGECK ? 0x958200 : 0xB7FE40, this, apPath);
	}

	RawShader* GetShader(const char* apcShaderName) {
		return ThisCall<RawShader*>(bGECK ? 0x957EC0 : 0xB7FAC0, this, apcShaderName);

	}
};

class BSShaderManager {
public:
	static ShaderBuffer* CreateShaderBuffer() {
		if (!GetShaderBuffer())
			SetShaderBuffer(new ShaderBuffer);

		return GetShaderBuffer();
	}

	static void DestroyShaderBuffer() {
		ShaderBuffer* pBuffer = GetShaderBuffer();
		if (pBuffer) {
			delete pBuffer;
			SetShaderBuffer(nullptr);
		}
	}

	static ShaderBuffer* GetShaderBuffer() {
		return *(ShaderBuffer**)(bGECK ? 0xF23EE8 : 0x11F9498);
	}

	static void SetShaderBuffer(ShaderBuffer* apBuffer) {
		*(ShaderBuffer**)(bGECK ? 0xF23EE8 : 0x11F9498) = apBuffer;
	}

	static NiDX9Renderer* GetRenderer() {
		return *(NiPointer<NiDX9Renderer>*)(bGECK ? 0xF23F58 : 0x11F9508);
	}
};
