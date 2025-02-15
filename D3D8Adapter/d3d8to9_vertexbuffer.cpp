
#include "stdafx.h"
#include "d3d8to9_vertexbuffer.h"

IDirect3DVertexBuffer9 *GetVertexBufferDelegate(Direct3DVertexBuffer8Adapter *adapter)
{
	return adapter->delegate;
}

Direct3DVertexBuffer8Adapter *CreateVertexBufferAdapter(IDirect3DVertexBuffer9 *delegate) {
	return new Direct3DVertexBuffer8Adapter(delegate);
}

void DeleteVertexBufferAdapter(Direct3DVertexBuffer8Adapter *adapter) {
	if (adapter->delegate) {
		adapter->delegate->Release();
	}
	delete adapter;
}

void SetVertexBufferDelegate(Direct3DVertexBuffer8Adapter *adapter, IDirect3DVertexBuffer9 *delegate) {
	// Free old delegate?
	adapter->delegate = delegate;
}

Direct3DVertexBuffer8Adapter::Direct3DVertexBuffer8Adapter(): delegate(nullptr)
{
}

Direct3DVertexBuffer8Adapter::~Direct3DVertexBuffer8Adapter()
{
}

/*** IUnknown methods ***/
HRESULT Direct3DVertexBuffer8Adapter::QueryInterface(THIS_ REFIID /*riid*/, void** /*ppvObj*/)
{
	return E_NOINTERFACE;
}

ULONG Direct3DVertexBuffer8Adapter::AddRef(THIS)
{
	return delegate->AddRef();
}

ULONG Direct3DVertexBuffer8Adapter::Release(THIS)
{
	// TODO: Free once Release == 0
	return delegate->Release();
}

/*** IDirect3DResource8 methods ***/
HRESULT Direct3DVertexBuffer8Adapter::GetDevice(THIS_ d3d8::IDirect3DDevice8** ppDevice)
{
	logger->error("Unsupported d3d function called: GetDevice");
	abort();
}

HRESULT Direct3DVertexBuffer8Adapter::SetPrivateData(THIS_ REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags)
{
	// TODO: Inspect Flags
	return delegate->SetPrivateData(refguid, pData, SizeOfData, Flags);
}

HRESULT Direct3DVertexBuffer8Adapter::GetPrivateData(THIS_ REFGUID refguid, void* pData, DWORD* pSizeOfData)
{
	return delegate->GetPrivateData(refguid, pData, pSizeOfData);
}

HRESULT Direct3DVertexBuffer8Adapter::FreePrivateData(THIS_ REFGUID refguid)
{
	return delegate->FreePrivateData(refguid);
}

DWORD Direct3DVertexBuffer8Adapter::SetPriority(THIS_ DWORD PriorityNew)
{
	return delegate->SetPriority(PriorityNew);
}

DWORD Direct3DVertexBuffer8Adapter::GetPriority(THIS)
{
	return delegate->GetPriority();
}

void Direct3DVertexBuffer8Adapter::PreLoad(THIS)
{
	delegate->PreLoad();
}

d3d8::D3DRESOURCETYPE Direct3DVertexBuffer8Adapter::GetType(THIS)
{
	// enums are compatible
	return (d3d8::D3DRESOURCETYPE) delegate->GetType();
}

HRESULT Direct3DVertexBuffer8Adapter::Lock(THIS_ UINT OffsetToLock, UINT SizeToLock, BYTE** ppbData, DWORD Flags)
{
	// TODO: Inspect flags
	return D3DLOG(delegate->Lock(OffsetToLock, SizeToLock, (void**)ppbData, Flags));
}

HRESULT Direct3DVertexBuffer8Adapter::Unlock(THIS)
{
	return D3DLOG(delegate->Unlock());
}

HRESULT Direct3DVertexBuffer8Adapter::GetDesc(THIS_ d3d8::D3DVERTEXBUFFER_DESC* pDesc)
{
	logger->error("Unsupported d3d function called: GetDesc");
	abort();
}
