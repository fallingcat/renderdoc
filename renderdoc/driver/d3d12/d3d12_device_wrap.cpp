/******************************************************************************
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 Baldur Karlsson
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 ******************************************************************************/

#include "d3d12_device.h"
#include "d3d12_command_list.h"
#include "d3d12_command_queue.h"
#include "d3d12_resources.h"

bool WrappedID3D12Device::Serialise_CreateCommandQueue(const D3D12_COMMAND_QUEUE_DESC *pDesc,
                                                       REFIID riid, void **ppCommandQueue)
{
  SERIALISE_ELEMENT_PTR(D3D12_COMMAND_QUEUE_DESC, Descriptor, pDesc);
  SERIALISE_ELEMENT(IID, guid, riid);
  SERIALISE_ELEMENT(ResourceId, Queue,
                    ((WrappedID3D12CommandQueue *)*ppCommandQueue)->GetResourceID());

  if(m_State == READING)
  {
    ID3D12CommandQueue *ret = NULL;
    HRESULT hr = m_pDevice->CreateCommandQueue(&Descriptor, guid, (void **)&ret);

    if(FAILED(hr))
    {
      RDCERR("Failed on resource serialise-creation, HRESULT: 0x%08x", hr);
    }
    else
    {
      ret = new WrappedID3D12CommandQueue(ret, this, m_pSerialiser);

      GetResourceManager()->AddLiveResource(Queue, ret);
    }
  }

  return true;
}

HRESULT WrappedID3D12Device::CreateCommandQueue(const D3D12_COMMAND_QUEUE_DESC *pDesc, REFIID riid,
                                                void **ppCommandQueue)
{
  if(ppCommandQueue == NULL)
    return m_pDevice->CreateCommandQueue(pDesc, riid, NULL);

  if(riid != __uuidof(ID3D12CommandQueue))
    return E_NOINTERFACE;

  ID3D12CommandQueue *real = NULL;
  HRESULT ret = m_pDevice->CreateCommandQueue(pDesc, riid, (void **)&real);

  if(SUCCEEDED(ret))
  {
    SCOPED_LOCK(m_D3DLock);

    WrappedID3D12CommandQueue *wrapped = new WrappedID3D12CommandQueue(real, this, m_pSerialiser);

    if(m_State >= WRITING)
    {
      SCOPED_SERIALISE_CONTEXT(CREATE_COMMAND_QUEUE);
      Serialise_CreateCommandQueue(pDesc, riid, (void **)&wrapped);

      m_DeviceRecord->AddChunk(scope.Get());
    }

    *ppCommandQueue = (ID3D12CommandQueue *)wrapped;
  }

  return ret;
}

HRESULT WrappedID3D12Device::CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE type, REFIID riid,
                                                    void **ppCommandAllocator)
{
  return m_pDevice->CreateCommandAllocator(type, riid, ppCommandAllocator);
}

bool WrappedID3D12Device::Serialise_CreateCommandList(UINT nodeMask, D3D12_COMMAND_LIST_TYPE type,
                                                      ID3D12CommandAllocator *pCommandAllocator,
                                                      ID3D12PipelineState *pInitialState,
                                                      REFIID riid, void **ppCommandList)
{
  SERIALISE_ELEMENT(UINT, Mask, nodeMask);
  SERIALISE_ELEMENT(D3D12_COMMAND_LIST_TYPE, ListType, type);
  SERIALISE_ELEMENT(ResourceId, Allocator, GetResID(pCommandAllocator));
  SERIALISE_ELEMENT(ResourceId, State, GetResID(pInitialState));
  SERIALISE_ELEMENT(IID, guid, riid);
  SERIALISE_ELEMENT(ResourceId, List,
                    ((WrappedID3D12GraphicsCommandList *)*ppCommandList)->GetResourceID());

  if(m_State == READING)
  {
    pCommandAllocator = (ID3D12CommandAllocator *)GetResourceManager()->GetLiveResource(Allocator);
    pInitialState = (ID3D12PipelineState *)GetResourceManager()->GetLiveResource(State);

    ID3D12GraphicsCommandList *ret = NULL;
    HRESULT hr = m_pDevice->CreateCommandList(Mask, ListType, pCommandAllocator, pInitialState,
                                              guid, (void **)&ret);

    if(FAILED(hr))
    {
      RDCERR("Failed on resource serialise-creation, HRESULT: 0x%08x", hr);
    }
    else
    {
      ret = new WrappedID3D12GraphicsCommandList(ret, this, m_pSerialiser);

      GetResourceManager()->AddLiveResource(List, ret);
    }
  }

  return true;
}

HRESULT WrappedID3D12Device::CreateCommandList(UINT nodeMask, D3D12_COMMAND_LIST_TYPE type,
                                               ID3D12CommandAllocator *pCommandAllocator,
                                               ID3D12PipelineState *pInitialState, REFIID riid,
                                               void **ppCommandList)
{
  if(ppCommandList == NULL)
    return m_pDevice->CreateCommandList(nodeMask, type, pCommandAllocator, pInitialState, riid, NULL);

  if(riid != __uuidof(ID3D12GraphicsCommandList))
    return E_NOINTERFACE;

  ID3D12GraphicsCommandList *real = NULL;
  HRESULT ret = m_pDevice->CreateCommandList(nodeMask, type, pCommandAllocator, pInitialState, riid,
                                             (void **)&real);

  if(SUCCEEDED(ret))
  {
    SCOPED_LOCK(m_D3DLock);

    WrappedID3D12GraphicsCommandList *wrapped =
        new WrappedID3D12GraphicsCommandList(real, this, m_pSerialiser);

    if(m_State >= WRITING)
    {
      SCOPED_SERIALISE_CONTEXT(CREATE_COMMAND_LIST);
      Serialise_CreateCommandList(nodeMask, type, pCommandAllocator, pInitialState, riid,
                                  (void **)&wrapped);

      m_DeviceRecord->AddChunk(scope.Get());
    }

    *ppCommandList = (ID3D12GraphicsCommandList *)wrapped;
  }

  return ret;
}

HRESULT WrappedID3D12Device::CreateGraphicsPipelineState(const D3D12_GRAPHICS_PIPELINE_STATE_DESC *pDesc,
                                                         REFIID riid, void **ppPipelineState)
{
  return m_pDevice->CreateGraphicsPipelineState(pDesc, riid, ppPipelineState);
}

HRESULT WrappedID3D12Device::CreateComputePipelineState(const D3D12_COMPUTE_PIPELINE_STATE_DESC *pDesc,
                                                        REFIID riid, void **ppPipelineState)
{
  return m_pDevice->CreateComputePipelineState(pDesc, riid, ppPipelineState);
}

HRESULT WrappedID3D12Device::CreateDescriptorHeap(const D3D12_DESCRIPTOR_HEAP_DESC *pDescriptorHeapDesc,
                                                  REFIID riid, void **ppvHeap)
{
  return m_pDevice->CreateDescriptorHeap(pDescriptorHeapDesc, riid, ppvHeap);
}

HRESULT WrappedID3D12Device::CreateRootSignature(UINT nodeMask, const void *pBlobWithRootSignature,
                                                 SIZE_T blobLengthInBytes, REFIID riid,
                                                 void **ppvRootSignature)
{
  return m_pDevice->CreateRootSignature(nodeMask, pBlobWithRootSignature, blobLengthInBytes, riid,
                                        ppvRootSignature);
}

void WrappedID3D12Device::CreateConstantBufferView(const D3D12_CONSTANT_BUFFER_VIEW_DESC *pDesc,
                                                   D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor)
{
  return m_pDevice->CreateConstantBufferView(pDesc, DestDescriptor);
}

void WrappedID3D12Device::CreateShaderResourceView(ID3D12Resource *pResource,
                                                   const D3D12_SHADER_RESOURCE_VIEW_DESC *pDesc,
                                                   D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor)
{
  return m_pDevice->CreateShaderResourceView(pResource, pDesc, DestDescriptor);
}

void WrappedID3D12Device::CreateUnorderedAccessView(ID3D12Resource *pResource,
                                                    ID3D12Resource *pCounterResource,
                                                    const D3D12_UNORDERED_ACCESS_VIEW_DESC *pDesc,
                                                    D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor)
{
  return m_pDevice->CreateUnorderedAccessView(pResource, pCounterResource, pDesc, DestDescriptor);
}

void WrappedID3D12Device::CreateRenderTargetView(ID3D12Resource *pResource,
                                                 const D3D12_RENDER_TARGET_VIEW_DESC *pDesc,
                                                 D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor)
{
  return m_pDevice->CreateRenderTargetView(Unwrap(pResource), pDesc, DestDescriptor);
}

void WrappedID3D12Device::CreateDepthStencilView(ID3D12Resource *pResource,
                                                 const D3D12_DEPTH_STENCIL_VIEW_DESC *pDesc,
                                                 D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor)
{
  return m_pDevice->CreateDepthStencilView(pResource, pDesc, DestDescriptor);
}

void WrappedID3D12Device::CreateSampler(const D3D12_SAMPLER_DESC *pDesc,
                                        D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor)
{
  return m_pDevice->CreateSampler(pDesc, DestDescriptor);
}

HRESULT WrappedID3D12Device::CreateCommittedResource(const D3D12_HEAP_PROPERTIES *pHeapProperties,
                                                     D3D12_HEAP_FLAGS HeapFlags,
                                                     const D3D12_RESOURCE_DESC *pResourceDesc,
                                                     D3D12_RESOURCE_STATES InitialResourceState,
                                                     const D3D12_CLEAR_VALUE *pOptimizedClearValue,
                                                     REFIID riidResource, void **ppvResource)
{
  return m_pDevice->CreateCommittedResource(pHeapProperties, HeapFlags, pResourceDesc,
                                            InitialResourceState, pOptimizedClearValue,
                                            riidResource, ppvResource);
}

HRESULT WrappedID3D12Device::CreateHeap(const D3D12_HEAP_DESC *pDesc, REFIID riid, void **ppvHeap)
{
  return m_pDevice->CreateHeap(pDesc, riid, ppvHeap);
}

HRESULT WrappedID3D12Device::CreatePlacedResource(ID3D12Heap *pHeap, UINT64 HeapOffset,
                                                  const D3D12_RESOURCE_DESC *pDesc,
                                                  D3D12_RESOURCE_STATES InitialState,
                                                  const D3D12_CLEAR_VALUE *pOptimizedClearValue,
                                                  REFIID riid, void **ppvResource)
{
  return m_pDevice->CreatePlacedResource(pHeap, HeapOffset, pDesc, InitialState,
                                         pOptimizedClearValue, riid, ppvResource);
}

HRESULT WrappedID3D12Device::CreateReservedResource(const D3D12_RESOURCE_DESC *pDesc,
                                                    D3D12_RESOURCE_STATES InitialState,
                                                    const D3D12_CLEAR_VALUE *pOptimizedClearValue,
                                                    REFIID riid, void **ppvResource)
{
  return m_pDevice->CreateReservedResource(pDesc, InitialState, pOptimizedClearValue, riid,
                                           ppvResource);
}

HRESULT WrappedID3D12Device::CreateFence(UINT64 InitialValue, D3D12_FENCE_FLAGS Flags, REFIID riid,
                                         void **ppFence)
{
  return m_pDevice->CreateFence(InitialValue, Flags, riid, ppFence);
}

HRESULT WrappedID3D12Device::CreateQueryHeap(const D3D12_QUERY_HEAP_DESC *pDesc, REFIID riid,
                                             void **ppvHeap)
{
  return m_pDevice->CreateQueryHeap(pDesc, riid, ppvHeap);
}

HRESULT WrappedID3D12Device::CreateCommandSignature(const D3D12_COMMAND_SIGNATURE_DESC *pDesc,
                                                    ID3D12RootSignature *pRootSignature,
                                                    REFIID riid, void **ppvCommandSignature)
{
  return m_pDevice->CreateCommandSignature(pDesc, pRootSignature, riid, ppvCommandSignature);
}

HRESULT WrappedID3D12Device::CreateSharedHandle(ID3D12DeviceChild *pObject,
                                                const SECURITY_ATTRIBUTES *pAttributes,
                                                DWORD Access, LPCWSTR Name, HANDLE *pHandle)
{
  return m_pDevice->CreateSharedHandle(pObject, pAttributes, Access, Name, pHandle);
}

void WrappedID3D12Device::CopyDescriptors(
    UINT NumDestDescriptorRanges, const D3D12_CPU_DESCRIPTOR_HANDLE *pDestDescriptorRangeStarts,
    const UINT *pDestDescriptorRangeSizes, UINT NumSrcDescriptorRanges,
    const D3D12_CPU_DESCRIPTOR_HANDLE *pSrcDescriptorRangeStarts,
    const UINT *pSrcDescriptorRangeSizes, D3D12_DESCRIPTOR_HEAP_TYPE DescriptorHeapsType)
{
  return m_pDevice->CopyDescriptors(NumDestDescriptorRanges, pDestDescriptorRangeStarts,
                                    pDestDescriptorRangeSizes, NumSrcDescriptorRanges,
                                    pSrcDescriptorRangeStarts, pSrcDescriptorRangeSizes,
                                    DescriptorHeapsType);
}

void WrappedID3D12Device::CopyDescriptorsSimple(UINT NumDescriptors,
                                                D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptorRangeStart,
                                                D3D12_CPU_DESCRIPTOR_HANDLE SrcDescriptorRangeStart,
                                                D3D12_DESCRIPTOR_HEAP_TYPE DescriptorHeapsType)
{
  return m_pDevice->CopyDescriptorsSimple(NumDescriptors, DestDescriptorRangeStart,
                                          SrcDescriptorRangeStart, DescriptorHeapsType);
}

HRESULT WrappedID3D12Device::OpenSharedHandle(HANDLE NTHandle, REFIID riid, void **ppvObj)
{
  return m_pDevice->OpenSharedHandle(NTHandle, riid, ppvObj);
}

HRESULT WrappedID3D12Device::OpenSharedHandleByName(LPCWSTR Name, DWORD Access, HANDLE *pNTHandle)
{
  return m_pDevice->OpenSharedHandleByName(Name, Access, pNTHandle);
}

HRESULT WrappedID3D12Device::MakeResident(UINT NumObjects, ID3D12Pageable *const *ppObjects)
{
  return m_pDevice->MakeResident(NumObjects, ppObjects);
}

HRESULT WrappedID3D12Device::Evict(UINT NumObjects, ID3D12Pageable *const *ppObjects)
{
  return m_pDevice->Evict(NumObjects, ppObjects);
}

//////////////////////////////////////////////////////////////////////
// we don't need to wrap any of these functions below

HRESULT WrappedID3D12Device::GetPrivateData(REFGUID guid, UINT *pDataSize, void *pData)
{
  return m_pDevice->GetPrivateData(guid, pDataSize, pData);
}

HRESULT WrappedID3D12Device::SetPrivateData(REFGUID guid, UINT DataSize, const void *pData)
{
  return m_pDevice->SetPrivateData(guid, DataSize, pData);
}

HRESULT WrappedID3D12Device::SetPrivateDataInterface(REFGUID guid, const IUnknown *pData)
{
  return m_pDevice->SetPrivateDataInterface(guid, pData);
}

HRESULT WrappedID3D12Device::SetName(LPCWSTR Name)
{
  return m_pDevice->SetName(Name);
}

UINT WrappedID3D12Device::GetNodeCount()
{
  return m_pDevice->GetNodeCount();
}

LUID WrappedID3D12Device::GetAdapterLuid()
{
  return m_pDevice->GetAdapterLuid();
}

void WrappedID3D12Device::GetResourceTiling(
    ID3D12Resource *pTiledResource, UINT *pNumTilesForEntireResource,
    D3D12_PACKED_MIP_INFO *pPackedMipDesc, D3D12_TILE_SHAPE *pStandardTileShapeForNonPackedMips,
    UINT *pNumSubresourceTilings, UINT FirstSubresourceTilingToGet,
    D3D12_SUBRESOURCE_TILING *pSubresourceTilingsForNonPackedMips)
{
  return m_pDevice->GetResourceTiling(
      pTiledResource, pNumTilesForEntireResource, pPackedMipDesc, pStandardTileShapeForNonPackedMips,
      pNumSubresourceTilings, FirstSubresourceTilingToGet, pSubresourceTilingsForNonPackedMips);
}

HRESULT WrappedID3D12Device::SetStablePowerState(BOOL Enable)
{
  return m_pDevice->SetStablePowerState(Enable);
}

HRESULT WrappedID3D12Device::CheckFeatureSupport(D3D12_FEATURE Feature, void *pFeatureSupportData,
                                                 UINT FeatureSupportDataSize)
{
  return m_pDevice->CheckFeatureSupport(Feature, pFeatureSupportData, FeatureSupportDataSize);
}

UINT WrappedID3D12Device::GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE DescriptorHeapType)
{
  return m_pDevice->GetDescriptorHandleIncrementSize(DescriptorHeapType);
}

D3D12_RESOURCE_ALLOCATION_INFO WrappedID3D12Device::GetResourceAllocationInfo(
    UINT visibleMask, UINT numResourceDescs, const D3D12_RESOURCE_DESC *pResourceDescs)
{
  return m_pDevice->GetResourceAllocationInfo(visibleMask, numResourceDescs, pResourceDescs);
}

D3D12_HEAP_PROPERTIES WrappedID3D12Device::GetCustomHeapProperties(UINT nodeMask,
                                                                   D3D12_HEAP_TYPE heapType)
{
  return m_pDevice->GetCustomHeapProperties(nodeMask, heapType);
}

HRESULT WrappedID3D12Device::GetDeviceRemovedReason()
{
  return m_pDevice->GetDeviceRemovedReason();
}

void WrappedID3D12Device::GetCopyableFootprints(const D3D12_RESOURCE_DESC *pResourceDesc,
                                                UINT FirstSubresource, UINT NumSubresources,
                                                UINT64 BaseOffset,
                                                D3D12_PLACED_SUBRESOURCE_FOOTPRINT *pLayouts,
                                                UINT *pNumRows, UINT64 *pRowSizeInBytes,
                                                UINT64 *pTotalBytes)
{
  return m_pDevice->GetCopyableFootprints(pResourceDesc, FirstSubresource, NumSubresources,
                                          BaseOffset, pLayouts, pNumRows, pRowSizeInBytes,
                                          pTotalBytes);
}
