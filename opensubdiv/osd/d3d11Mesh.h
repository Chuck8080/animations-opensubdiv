//
//   Copyright 2013 Pixar
//
//   Licensed under the Apache License, Version 2.0 (the "Apache License")
//   with the following modification; you may not use this file except in
//   compliance with the Apache License and the following modification to it:
//   Section 6. Trademarks. is deleted and replaced with:
//
//   6. Trademarks. This License does not grant permission to use the trade
//      names, trademarks, service marks, or product names of the Licensor
//      and its affiliates, except as required to comply with Section 4(c) of
//      the License and to reproduce the content of the NOTICE file.
//
//   You may obtain a copy of the Apache License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the Apache License with the above modification is
//   distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
//   KIND, either express or implied. See the Apache License for the specific
//   language governing permissions and limitations under the Apache License.
//

#ifndef OSD_D3D11MESH_H
#define OSD_D3D11MESH_H

#include "../version.h"

#include "../osd/mesh.h"
#include "../osd/d3d11ComputeController.h"
#include "../osd/d3d11DrawContext.h"
#include "../osd/d3d11VertexBuffer.h"

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {

typedef OsdMeshInterface<OsdD3D11DrawContext> OsdD3D11MeshInterface;

template <class VERTEX_BUFFER, class COMPUTE_CONTROLLER>
class OsdMesh<VERTEX_BUFFER, COMPUTE_CONTROLLER, OsdD3D11DrawContext> : public OsdD3D11MeshInterface {
public:
    typedef VERTEX_BUFFER VertexBuffer;
    typedef COMPUTE_CONTROLLER ComputeController;
    typedef typename ComputeController::ComputeContext ComputeContext;
    typedef OsdD3D11DrawContext DrawContext;
    typedef typename DrawContext::VertexBufferBinding VertexBufferBinding;

    OsdMesh(ComputeController * computeController,
            FarRefineTables * refTables,
            int numVertexElements,
            int numVaryingElements,
            int level,
            OsdMeshBitset bits,
            ID3D11DeviceContext *d3d11DeviceContext) :

            _refTables(refTables),
            _vertexBuffer(0),
            _varyingBuffer(0),
            _computeContext(0),
            _computeController(computeController),
            _drawContext(0),
            _d3d11DeviceContext(d3d11DeviceContext)
    {
        OsdD3D11MeshInterface::refineMesh(*_refTables, level, bits.test(MeshAdaptive));

        int numElements =
            initializeVertexBuffers(numVertexElements, numVaryingElements, bits);

        initializeComputeContext(numVertexElements, numVaryingElements);

        initializeDrawContext(numElements, bits);
    }

    OsdMesh(ComputeController * computeController,
            FarRefineTables * refTables,
            VertexBuffer * vertexBuffer,
            VertexBuffer * varyingBuffer,
            ComputeContext * computeContext,
            DrawContext * drawContext,
            ID3D11DeviceContext *d3d11DeviceContext) :

            _refTables(refTables),
            _vertexBuffer(vertexBuffer),
            _varyingBuffer(varyingBuffer),
            _computeContext(computeContext),
            _computeController(computeController),
            _drawContext(drawContext),
            _d3d11DeviceContext(d3d11DeviceContext)
    {
        _drawContext->UpdateVertexTexture(_vertexBuffer, _d3d11DeviceContext);
    }

    virtual ~OsdMesh() {
        delete _refTables;
        delete _vertexBuffer;
        delete _varyingBuffer;
        delete _computeContext;
        delete _drawContext;
    }

    virtual int GetNumVertices() const {
        assert(_refTables);
        return OsdD3D11MeshInterface::getNumVertices(*_refTables);
    }

    virtual void UpdateVertexBuffer(float const *vertexData, int startVertex, int numVerts) {
        _vertexBuffer->UpdateData(vertexData, startVertex, numVerts, _d3d11DeviceContext);
    }
    virtual void UpdateVaryingBuffer(float const *varyingData, int startVertex, int numVerts) {
        _varyingBuffer->UpdateData(varyingData, startVertex, numVerts, _d3d11DeviceContext);
    }
    virtual void Refine() {
        _computeController->Compute(_computeContext, _kernelBatches, _vertexBuffer, _varyingBuffer);
    }
    virtual void Refine(OsdVertexBufferDescriptor const *vertexDesc,
                        OsdVertexBufferDescriptor const *varyingDesc,
                        bool interleaved) {
        _computeController->Compute(_computeContext, _kernelBatches,
                                    _vertexBuffer, (interleaved ? _vertexBuffer : _varyingBuffer),
                                    vertexDesc, varyingDesc);
    }
    virtual void Synchronize() {
        _computeController->Synchronize();
    }
    virtual VertexBufferBinding BindVertexBuffer() {
        return _vertexBuffer->BindD3D11Buffer(_d3d11DeviceContext);
    }
    virtual VertexBufferBinding BindVaryingBuffer() {
        return _varyingBuffer->BindD3D11Buffer(_d3d11DeviceContext);
    }
    virtual DrawContext * GetDrawContext() {
        return _drawContext;
    }
    virtual VertexBuffer * GetVertexBuffer() {
        return _vertexBuffer;
    }
    virtual VertexBuffer * GetVaryingBuffer() {
        return _varyingBuffer;
    }

    virtual FarRefineTables const * GetRefineTables() const {
        return _refTables;
    }


private:

    void initializeComputeContext(int numVertexElements,
        int numVaryingElements ) {

        assert(_refTables);

        FarStencilTablesFactory::Options options;
        options.generateOffsets=true;
        options.generateAllLevels=_refTables->IsUniform() ? false : true;

        FarStencilTables const * vertexStencils=0, * varyingStencils=0;

        if (numVertexElements>0) {

            vertexStencils = FarStencilTablesFactory::Create(*_refTables, options);

            _kernelBatches.push_back(FarStencilTablesFactory::Create(*vertexStencils));
        }

        if (numVaryingElements>0) {

            options.interpolationMode = FarStencilTablesFactory::INTERPOLATE_VARYING;

            varyingStencils = FarStencilTablesFactory::Create(*_refTables, options);
        }

        _computeContext = ComputeContext::Create(vertexStencils, varyingStencils);

        delete vertexStencils;
        delete varyingStencils;
    }

    void initializeDrawContext(int numElements, OsdMeshBitset bits) {

        assert(_refTables and _vertexBuffer);

        FarPatchTables const * patchTables =
            FarPatchTablesFactory::Create(*_refTables);

        _drawContext = DrawContext::Create(
            patchTables, _d3d11DeviceContext, numElements, bits.test(MeshFVarData));

        _drawContext->UpdateVertexTexture(_vertexBuffer, _d3d11DeviceContext);

        delete patchTables;
    }

    int initializeVertexBuffers(int numVertexElements,
        int numVaryingElements, OsdMeshBitset bits) {

        ID3D11Device * pd3d11Device;
        _d3d11DeviceContext->GetDevice(&pd3d11Device);

        int numVertices = OsdD3D11MeshInterface::getNumVertices(*_refTables);

        int numElements = numVertexElements +
            (bits.test(MeshInterleaveVarying) ? numVaryingElements : 0);

        if (numVertexElements) {

            _vertexBuffer =
                VertexBuffer::Create(numElements, numVertices, pd3d11Device);
        }

        if (numVaryingElements>0 and (not bits.test(MeshInterleaveVarying))) {
            _varyingBuffer =
                VertexBuffer::Create(numVaryingElements, numVertices, pd3d11Device);
        }
        return numElements;
    }

    FarRefineTables * _refTables;
    FarKernelBatchVector _kernelBatches;

    VertexBuffer *_vertexBuffer;
    VertexBuffer *_varyingBuffer;

    ComputeContext *_computeContext;
    ComputeController *_computeController;
    DrawContext *_drawContext;

    ID3D11DeviceContext *_d3d11DeviceContext;
};

template <>
class OsdMesh<OsdD3D11VertexBuffer, OsdD3D11ComputeController, OsdD3D11DrawContext> : public OsdD3D11MeshInterface {
public:
    typedef OsdD3D11VertexBuffer VertexBuffer;
    typedef OsdD3D11ComputeController ComputeController;
    typedef ComputeController::ComputeContext ComputeContext;
    typedef OsdD3D11DrawContext DrawContext;
    typedef DrawContext::VertexBufferBinding VertexBufferBinding;

    OsdMesh(ComputeController * computeController,
            FarRefineTables * refTables,
            int numVertexElements,
            int numVaryingElements,
            int level,
            OsdMeshBitset bits,
            ID3D11DeviceContext *d3d11DeviceContext) :

            _refTables(refTables),
            _vertexBuffer(0),
            _varyingBuffer(0),
            _computeContext(0),
            _computeController(computeController),
            _drawContext(0),
            _d3d11DeviceContext(d3d11DeviceContext)
    {
        OsdD3D11MeshInterface::refineMesh(*_refTables, level, bits.test(MeshAdaptive));

        int numElements =
            initializeVertexBuffers(numVertexElements, numVaryingElements, bits);

        initializeComputeContext(numVertexElements, numVaryingElements);

        initializeDrawContext(numElements, bits);
    }

    OsdMesh(ComputeController * computeController,
            FarRefineTables * refTables,
            VertexBuffer * vertexBuffer,
            VertexBuffer * varyingBuffer,
            ComputeContext * computeContext,
            DrawContext * drawContext,
            ID3D11DeviceContext *d3d11DeviceContext) :

            _refTables(refTables),
            _vertexBuffer(vertexBuffer),
            _varyingBuffer(varyingBuffer),
            _computeContext(computeContext),
            _computeController(computeController),
            _drawContext(drawContext),
            _d3d11DeviceContext(d3d11DeviceContext)
    {
        _drawContext->UpdateVertexTexture(_vertexBuffer, _d3d11DeviceContext);
    }

    virtual ~OsdMesh() {
        delete _refTables;
        delete _vertexBuffer;
        delete _varyingBuffer;
        delete _computeContext;
        delete _drawContext;
    }

    virtual int GetNumVertices() const { return _refTables->GetNumVerticesTotal(); }

    virtual void UpdateVertexBuffer(float const *vertexData, int startVertex, int numVerts) {
        _vertexBuffer->UpdateData(vertexData, startVertex, numVerts, _d3d11DeviceContext);
    }
    virtual void UpdateVaryingBuffer(float const *varyingData, int startVertex, int numVerts) {
        _varyingBuffer->UpdateData(varyingData, startVertex, numVerts, _d3d11DeviceContext);
    }
    virtual void Refine() {
        _computeController->Compute(_computeContext, _kernelBatches, _vertexBuffer, _varyingBuffer);
    }
    virtual void Refine(OsdVertexBufferDescriptor const *vertexDesc,
                        OsdVertexBufferDescriptor const *varyingDesc,
                        bool interleaved) {
        _computeController->Compute(_computeContext, _kernelBatches,
                                    _vertexBuffer, (interleaved ? _vertexBuffer : _varyingBuffer),
                                    vertexDesc, varyingDesc);
    }
    virtual void Synchronize() {
        _computeController->Synchronize();
    }
    virtual VertexBufferBinding BindVertexBuffer() {
        return _vertexBuffer->BindD3D11Buffer(_d3d11DeviceContext);
    }
    virtual VertexBufferBinding BindVaryingBuffer() {
        return _varyingBuffer->BindD3D11Buffer(_d3d11DeviceContext);
    }
    virtual DrawContext * GetDrawContext() {
        return _drawContext;
    }
    virtual VertexBuffer * GetVertexBuffer() {
        return _vertexBuffer;
    }
    virtual VertexBuffer * GetVaryingBuffer() {
        return _varyingBuffer;
    }


    virtual FarRefineTables const * GetRefineTables() const {
        return _refTables;
    }

private:


    void initializeComputeContext(int numVertexElements,
        int numVaryingElements ) {

        assert(_refTables);

        FarStencilTablesFactory::Options options;
        options.generateOffsets=true;
        options.generateAllLevels=_refTables->IsUniform() ? false : true;

        FarStencilTables const * vertexStencils=0, * varyingStencils=0;

        if (numVertexElements>0) {

            vertexStencils = FarStencilTablesFactory::Create(*_refTables, options);

            _kernelBatches.push_back(FarStencilTablesFactory::Create(*vertexStencils));
        }

        if (numVaryingElements>0) {

            options.interpolationMode = FarStencilTablesFactory::INTERPOLATE_VARYING;

            varyingStencils = FarStencilTablesFactory::Create(*_refTables, options);
        }

        _computeContext =
            ComputeContext::Create(_d3d11DeviceContext, vertexStencils, varyingStencils);

        delete vertexStencils;
        delete varyingStencils;
    }

    void initializeDrawContext(int numElements, OsdMeshBitset bits) {

        assert(_refTables and _vertexBuffer);

        FarPatchTables const * patchTables =
            FarPatchTablesFactory::Create(*_refTables);

        _drawContext = DrawContext::Create(
            patchTables, _d3d11DeviceContext, numElements, bits.test(MeshFVarData));

        _drawContext->UpdateVertexTexture(_vertexBuffer, _d3d11DeviceContext);

        delete patchTables;
    }

    int initializeVertexBuffers(int numVertexElements,
        int numVaryingElements, OsdMeshBitset bits) {

        ID3D11Device * pd3d11Device;
        _d3d11DeviceContext->GetDevice(&pd3d11Device);

        int numVertices = OsdD3D11MeshInterface::getNumVertices(*_refTables);

        int numElements = numVertexElements +
            (bits.test(MeshInterleaveVarying) ? numVaryingElements : 0);

        if (numVertexElements) {
            _vertexBuffer =
                VertexBuffer::Create(numElements, numVertices, pd3d11Device);
        }

        if (numVaryingElements>0 and (not bits.test(MeshInterleaveVarying))) {
            _varyingBuffer =
                VertexBuffer::Create(numVaryingElements, numVertices, pd3d11Device);
        }
        return numElements;
    }

    FarRefineTables * _refTables;
    FarKernelBatchVector _kernelBatches;

    VertexBuffer *_vertexBuffer;
    VertexBuffer *_varyingBuffer;

    ComputeContext *_computeContext;
    ComputeController *_computeController;
    DrawContext *_drawContext;

    ID3D11DeviceContext *_d3d11DeviceContext;
};


}  // end namespace OPENSUBDIV_VERSION
using namespace OPENSUBDIV_VERSION;

}  // end namespace OpenSubdiv

#endif  // OSD_D3D11MESH_H
