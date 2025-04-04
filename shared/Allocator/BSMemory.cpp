#include "BSMemory.hpp"

// -------------------------------------------------------------------------
// Internal globals and functions
// -------------------------------------------------------------------------
void*	pMemoryManager = nullptr;
namespace CurrentMemManager {
	void* (__thiscall*	Allocate)(void* pThis, size_t stSize) = nullptr;
	void* (__thiscall*	ReAllocate)(void* pThis, void* pvMem, size_t stSize) = nullptr;
	void(__thiscall*	Deallocate)(void* pThis, void* pvMem) = nullptr;
	size_t(__thiscall*	Size)(void* pThis, void* pvMem) = nullptr;
}

void	BSAllocatorInitializer();
void	CreateHeapIfNotExisting(uint32_t auiHeapAddress, uint32_t auiCallAddress);
int		(__cdecl* CreateHeap)(uint32_t aeSerialize);
int		CreateHeapStub(uint32_t aeSerialize) { return 1; }
bool	bInitialized = false;

// -------------------------------------------------------------------------
// Functions made to be used by the user
// These functions use game's memory manager to manage memory
// They are used to replace new and delete operators
// -------------------------------------------------------------------------
void* BSNew(size_t stSize) {
	if (!bInitialized)
		BSAllocatorInitializer();

	return CurrentMemManager::Allocate(pMemoryManager, stSize);
}

void* BSNewAligned(size_t stAlign, size_t stSize) {
	uint8_t* pMemory = static_cast<uint8_t*>(BSNew(stSize + stAlign));
	uint32_t uiAlignment = stAlign - (reinterpret_cast<uint8_t>(pMemory) & (stAlign - 1));
	pMemory[uint8_t(uiAlignment) - 1] = uiAlignment;
	return &pMemory[uint8_t(uiAlignment)];
}

void* BSReallocate(void* pvMem, size_t stSize) {
	return CurrentMemManager::ReAllocate(pMemoryManager, pvMem, stSize);
}

void BSFree(void* pvMem) {
	CurrentMemManager::Deallocate(pMemoryManager, pvMem);
}

SIZE_T BSSize(void* pvMem) {
	return CurrentMemManager::Size(pMemoryManager, pvMem);
}

// -------------------------------------------------------------------------
// Functions made to initialize the allocator
// Compatible with both game and GECK
// -------------------------------------------------------------------------

// This function is used to create game's heap if it doesn't exist
// It's possible to load the plugin before game is even initialized
// In those cases, malloc fails due to lack of heap - that's why we need to create it manually
_declspec(noinline) void CreateHeapIfNotExisting(uint32_t auiHeapAddress, uint32_t auiCallAddress) {
	if (*(HANDLE*)auiHeapAddress)
		return;

	CreateHeap(true);

	bInitialized = true;

	DWORD oldProtect;
	VirtualProtect((void*)auiCallAddress, 4, PAGE_EXECUTE_READWRITE, &oldProtect);
	*(uint32_t*)(auiCallAddress + 1) = uint32_t(CreateHeapStub) - auiCallAddress - 5;
	VirtualProtect((void*)auiCallAddress, 4, oldProtect, &oldProtect);
}

// This function sets up correct addresses based on the program
_declspec(noinline) void BSAllocatorInitializer() {
	if (*(uint8_t*)0x401190 != 0x55) {
		// Is GECK
		pMemoryManager					= (void*)0xF21B5C;
		CurrentMemManager::Allocate		= (void* (__thiscall*)(void*, size_t))0x8540A0;
		CurrentMemManager::ReAllocate	= (void* (__thiscall*)(void*, void*, size_t))0x8543B0;
		CurrentMemManager::Deallocate	= (void(__thiscall*)(void*, void*))0x8542C0;
		CurrentMemManager::Size			= (size_t(__thiscall*)(void*, void*))0x854720;
		CreateHeap						= (int(__cdecl*)(uint32_t))0xC770C3;

		CreateHeapIfNotExisting(0xF9907C, 0xC62B21);
	}
	else {
		pMemoryManager					= (void*)0x11F6238;
		CurrentMemManager::Allocate		= (void* (__thiscall*)(void*, size_t))0xAA3E40;
		CurrentMemManager::ReAllocate	= (void* (__thiscall*)(void*, void*, size_t))0xAA4150;
		CurrentMemManager::Deallocate	= (void(__thiscall*)(void*, void*))0xAA4060;
		CurrentMemManager::Size			= (size_t(__thiscall*)(void*, void*))0xAA44C0;
		CreateHeap						= (int(__cdecl*)(uint32_t))0xEDDB6A;

		CreateHeapIfNotExisting(0x12705BC, 0xECC3CB);
	}
}