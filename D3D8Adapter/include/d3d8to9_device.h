#pragma once

#include "d3d.h"
#include "d3d8adapter.h"

struct Direct3DDevice8Adapter : public d3d8::IDirect3DDevice8
{
	STDMETHOD(QueryInterface)(THIS_ REFIID /*riid*/, void** /*ppvObj*/);
	STDMETHOD_(ULONG, AddRef)(THIS);
	STDMETHOD_(ULONG, Release)(THIS);

	/*** IDirect3DDevice8 methods ***/
	STDMETHOD(TestCooperativeLevel)(THIS);
	STDMETHOD_(UINT, GetAvailableTextureMem)(THIS);
	STDMETHOD(ResourceManagerDiscardBytes)(THIS_ DWORD Bytes);
	STDMETHOD(GetDirect3D)(THIS_ d3d8::IDirect3D8** ppD3D8);
	STDMETHOD(GetDeviceCaps)(THIS_ d3d8::D3DCAPS8* pCaps);
	STDMETHOD(GetDisplayMode)(THIS_ d3d8::D3DDISPLAYMODE* pMode);
	STDMETHOD(GetCreationParameters)(THIS_ d3d8::D3DDEVICE_CREATION_PARAMETERS* pParameters);
	STDMETHOD(SetCursorProperties)(THIS_ UINT XHotSpot, UINT YHotSpot, d3d8::IDirect3DSurface8* pCursorBitmap);
	STDMETHOD_(void, SetCursorPosition)(THIS_ int X, int Y, DWORD Flags);
	STDMETHOD_(BOOL, ShowCursor)(THIS_ BOOL bShow);
	STDMETHOD(CreateAdditionalSwapChain)(THIS_ d3d8::D3DPRESENT_PARAMETERS* pPresentationParameters, d3d8::IDirect3DSwapChain8** pSwapChain);
	STDMETHOD(Reset)(THIS_ d3d8::D3DPRESENT_PARAMETERS* pPresentationParameters);
	STDMETHOD(Present)(THIS_ CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion);
	STDMETHOD(GetBackBuffer)(THIS_ UINT BackBuffer, d3d8::D3DBACKBUFFER_TYPE Type, d3d8::IDirect3DSurface8** ppBackBuffer);
	STDMETHOD(GetRasterStatus)(THIS_ d3d8::D3DRASTER_STATUS* pRasterStatus);
	STDMETHOD_(void, SetGammaRamp)(THIS_ DWORD Flags, CONST d3d8::D3DGAMMARAMP* pRamp);
	STDMETHOD_(void, GetGammaRamp)(THIS_ d3d8::D3DGAMMARAMP* pRamp);
	STDMETHOD(CreateTexture)(THIS_ UINT Width, UINT Height, UINT Levels, DWORD Usage, d3d8::D3DFORMAT Format, d3d8::D3DPOOL Pool, d3d8::IDirect3DTexture8** ppTexture);
	STDMETHOD(CreateVolumeTexture)(THIS_ UINT Width, UINT Height, UINT Depth, UINT Levels, DWORD Usage, d3d8::D3DFORMAT Format, d3d8::D3DPOOL Pool, d3d8::IDirect3DVolumeTexture8** ppVolumeTexture);
	STDMETHOD(CreateCubeTexture)(THIS_ UINT EdgeLength, UINT Levels, DWORD Usage, d3d8::D3DFORMAT Format, d3d8::D3DPOOL Pool, d3d8::IDirect3DCubeTexture8** ppCubeTexture);
	STDMETHOD(CreateVertexBuffer)(THIS_ UINT Length, DWORD Usage, DWORD FVF, d3d8::D3DPOOL Pool, d3d8::IDirect3DVertexBuffer8** ppVertexBuffer);
	STDMETHOD(CreateIndexBuffer)(THIS_ UINT Length, DWORD Usage, d3d8::D3DFORMAT Format, d3d8::D3DPOOL Pool, d3d8::IDirect3DIndexBuffer8** ppIndexBuffer);
	STDMETHOD(CreateRenderTarget)(THIS_ UINT Width, UINT Height, d3d8::D3DFORMAT Format, d3d8::D3DMULTISAMPLE_TYPE MultiSample, BOOL Lockable, d3d8::IDirect3DSurface8** ppSurface);
	STDMETHOD(CreateDepthStencilSurface)(THIS_ UINT Width, UINT Height, d3d8::D3DFORMAT Format, d3d8::D3DMULTISAMPLE_TYPE MultiSample, d3d8::IDirect3DSurface8** ppSurface);
	STDMETHOD(CreateImageSurface)(THIS_ UINT Width, UINT Height, d3d8::D3DFORMAT Format, d3d8::IDirect3DSurface8** ppSurface);
	STDMETHOD(CopyRects)(THIS_ d3d8::IDirect3DSurface8* pSourceSurface, CONST RECT* pSourceRectsArray, UINT cRects, d3d8::IDirect3DSurface8* pDestinationSurface, CONST POINT* pDestPointsArray);
	STDMETHOD(UpdateTexture)(THIS_ d3d8::IDirect3DBaseTexture8* pSourceTexture, d3d8::IDirect3DBaseTexture8* pDestinationTexture);
	STDMETHOD(GetFrontBuffer)(THIS_ d3d8::IDirect3DSurface8* pDestSurface);
	STDMETHOD(SetRenderTarget)(THIS_ d3d8::IDirect3DSurface8* pRenderTarget, d3d8::IDirect3DSurface8* pNewZStencil);
	STDMETHOD(GetRenderTarget)(THIS_ d3d8::IDirect3DSurface8** ppRenderTarget);
	STDMETHOD(GetDepthStencilSurface)(THIS_ d3d8::IDirect3DSurface8** ppZStencilSurface);
	STDMETHOD(BeginScene)(THIS);
	STDMETHOD(EndScene)(THIS);
	STDMETHOD(Clear)(THIS_ DWORD Count, CONST d3d8::D3DRECT* pRects, DWORD Flags, d3d8::D3DCOLOR Color, float Z, DWORD Stencil);
	STDMETHOD(SetTransform)(THIS_ d3d8::D3DTRANSFORMSTATETYPE State, CONST d3d8::D3DMATRIX* pMatrix);
	STDMETHOD(GetTransform)(THIS_ d3d8::D3DTRANSFORMSTATETYPE State, d3d8::D3DMATRIX* pMatrix);
	STDMETHOD(MultiplyTransform)(THIS_ d3d8::D3DTRANSFORMSTATETYPE State, CONST d3d8::D3DMATRIX* pMatrix);
	STDMETHOD(SetViewport)(THIS_ CONST d3d8::D3DVIEWPORT8* pViewport);
	STDMETHOD(GetViewport)(THIS_ d3d8::D3DVIEWPORT8* pViewport);
	STDMETHOD(SetMaterial)(THIS_ CONST d3d8::D3DMATERIAL8* pMaterial);
	STDMETHOD(GetMaterial)(THIS_ d3d8::D3DMATERIAL8* pMaterial);
	STDMETHOD(SetLight)(THIS_ DWORD Index, CONST d3d8::D3DLIGHT8* Light);
	STDMETHOD(GetLight)(THIS_ DWORD Index, d3d8::D3DLIGHT8* Light);
	STDMETHOD(LightEnable)(THIS_ DWORD Index, BOOL Enable);
	STDMETHOD(GetLightEnable)(THIS_ DWORD Index, BOOL* pEnable);
	STDMETHOD(SetClipPlane)(THIS_ DWORD Index, CONST float* pPlane);
	STDMETHOD(GetClipPlane)(THIS_ DWORD Index, float* pPlane);
	STDMETHOD(SetRenderState)(THIS_ d3d8::D3DRENDERSTATETYPE State, DWORD Value);
	STDMETHOD(GetRenderState)(THIS_ d3d8::D3DRENDERSTATETYPE State, DWORD* pValue);
	STDMETHOD(BeginStateBlock)(THIS);
	STDMETHOD(EndStateBlock)(THIS_ DWORD* pToken);
	STDMETHOD(ApplyStateBlock)(THIS_ DWORD Token);
	STDMETHOD(CaptureStateBlock)(THIS_ DWORD Token);
	STDMETHOD(DeleteStateBlock)(THIS_ DWORD Token);
	STDMETHOD(CreateStateBlock)(THIS_ d3d8::D3DSTATEBLOCKTYPE Type, DWORD* pToken);
	STDMETHOD(SetClipStatus)(THIS_ CONST d3d8::D3DCLIPSTATUS8* pClipStatus);
	STDMETHOD(GetClipStatus)(THIS_ d3d8::D3DCLIPSTATUS8* pClipStatus);
	STDMETHOD(GetTexture)(THIS_ DWORD Stage, d3d8::IDirect3DBaseTexture8** ppTexture);
	STDMETHOD(SetTexture)(THIS_ DWORD Stage, d3d8::IDirect3DBaseTexture8* pTexture);
	STDMETHOD(GetTextureStageState)(THIS_ DWORD Stage, d3d8::D3DTEXTURESTAGESTATETYPE Type, DWORD* pValue);
	STDMETHOD(SetTextureStageState)(THIS_ DWORD Stage, d3d8::D3DTEXTURESTAGESTATETYPE Type, DWORD Value);
	STDMETHOD(ValidateDevice)(THIS_ DWORD* pNumPasses);
	STDMETHOD(GetInfo)(THIS_ DWORD DevInfoID, void* pDevInfoStruct, DWORD DevInfoStructSize);
	STDMETHOD(SetPaletteEntries)(THIS_ UINT PaletteNumber, CONST PALETTEENTRY* pEntries);
	STDMETHOD(GetPaletteEntries)(THIS_ UINT PaletteNumber, PALETTEENTRY* pEntries);
	STDMETHOD(SetCurrentTexturePalette)(THIS_ UINT PaletteNumber);
	STDMETHOD(GetCurrentTexturePalette)(THIS_ UINT* PaletteNumber);
	STDMETHOD(DrawPrimitive)(THIS_ d3d8::D3DPRIMITIVETYPE type, UINT StartVertex, UINT PrimitiveCount);
	STDMETHOD(DrawIndexedPrimitive)(THIS_ d3d8::D3DPRIMITIVETYPE type, UINT minIndex, UINT NumVertices, UINT startIndex, UINT primCount);
	STDMETHOD(DrawPrimitiveUP)(THIS_ d3d8::D3DPRIMITIVETYPE PrimitiveType, UINT PrimitiveCount, CONST void* pVertexStreamZeroData, UINT VertexStreamZeroStride);
	STDMETHOD(DrawIndexedPrimitiveUP)(THIS_ d3d8::D3DPRIMITIVETYPE PrimitiveType, UINT MinVertexIndex, UINT NumVertexIndices, UINT PrimitiveCount, CONST void* pIndexData, d3d8::D3DFORMAT IndexDataFormat, CONST void* pVertexStreamZeroData, UINT VertexStreamZeroStride);
	STDMETHOD(ProcessVertices)(THIS_ UINT SrcStartIndex, UINT DestIndex, UINT VertexCount, d3d8::IDirect3DVertexBuffer8* pDestBuffer, DWORD Flags);
	STDMETHOD(CreateVertexShader)(THIS_ CONST DWORD* pDeclaration, CONST DWORD* pFunction, DWORD* pHandle, DWORD Usage);
	STDMETHOD(SetVertexShader)(THIS_ DWORD Handle);
	STDMETHOD(GetVertexShader)(THIS_ DWORD* pHandle);
	STDMETHOD(DeleteVertexShader)(THIS_ DWORD Handle);
	STDMETHOD(SetVertexShaderConstant)(THIS_ DWORD Register, CONST void* pConstantData, DWORD ConstantCount);
	STDMETHOD(GetVertexShaderConstant)(THIS_ DWORD Register, void* pConstantData, DWORD ConstantCount);
	STDMETHOD(GetVertexShaderDeclaration)(THIS_ DWORD Handle, void* pData, DWORD* pSizeOfData);
	STDMETHOD(GetVertexShaderFunction)(THIS_ DWORD Handle, void* pData, DWORD* pSizeOfData);
	STDMETHOD(SetStreamSource)(THIS_ UINT StreamNumber, d3d8::IDirect3DVertexBuffer8* pStreamData, UINT Stride);
	STDMETHOD(GetStreamSource)(THIS_ UINT StreamNumber, d3d8::IDirect3DVertexBuffer8** ppStreamData, UINT* pStride);
	STDMETHOD(SetIndices)(THIS_ d3d8::IDirect3DIndexBuffer8* pIndexData, UINT BaseVertexIndex);
	STDMETHOD(GetIndices)(THIS_ d3d8::IDirect3DIndexBuffer8** ppIndexData, UINT* pBaseVertexIndex);
	STDMETHOD(CreatePixelShader)(THIS_ CONST DWORD* pFunction, DWORD* pHandle);
	STDMETHOD(SetPixelShader)(THIS_ DWORD Handle);
	STDMETHOD(GetPixelShader)(THIS_ DWORD* pHandle);
	STDMETHOD(DeletePixelShader)(THIS_ DWORD Handle);
	STDMETHOD(SetPixelShaderConstant)(THIS_ DWORD Register, CONST void* pConstantData, DWORD ConstantCount);
	STDMETHOD(GetPixelShaderConstant)(THIS_ DWORD Register, void* pConstantData, DWORD ConstantCount);
	STDMETHOD(GetPixelShaderFunction)(THIS_ DWORD Handle, void* pData, DWORD* pSizeOfData);
	STDMETHOD(DrawRectPatch)(THIS_ UINT Handle, CONST float* pNumSegs, CONST d3d8::D3DRECTPATCH_INFO* pRectPatchInfo);
	STDMETHOD(DrawTriPatch)(THIS_ UINT Handle, CONST float* pNumSegs, CONST d3d8::D3DTRIPATCH_INFO* pTriPatchInfo);
	STDMETHOD(DeletePatch)(THIS_ UINT Handle);

	Direct3DDevice8Adapter();
	virtual ~Direct3DDevice8Adapter();

	IDirect3DDevice9Ex* delegate;
	Direct3DTexture8Adapter* stages[16];
	Direct3DVertexBuffer8Adapter* streamSources[16];
	Direct3DIndexBuffer8Adapter* indices;
};
