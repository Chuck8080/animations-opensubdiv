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

#include "../far/stencilTables.h"
#include "../osd/cpuComputeContext.h"
#include "../osd/cpuComputeController.h"
#include "../osd/cpuKernel.h"

#include <cassert>

namespace OpenSubdiv {
namespace OPENSUBDIV_VERSION {


OsdCpuComputeController::OsdCpuComputeController() {
}

OsdCpuComputeController::~OsdCpuComputeController() {
}

void
OsdCpuComputeController::Synchronize() {
}

void
OsdCpuComputeController::ApplyStencilTableKernel(
    FarKernelBatch const &batch, ComputeContext const *context) const {

    assert(context);

    FarStencilTables const * vertexStencils = context->GetVertexStencilTables();

    if (vertexStencils and _currentBindState.vertexBuffer) {

        OsdVertexBufferDescriptor const & desc = _currentBindState.vertexDesc;

        float const * srcBuffer = _currentBindState.vertexBuffer + desc.offset;

        float * destBuffer = _currentBindState.vertexBuffer + desc.offset +
            vertexStencils->GetNumControlVertices() * desc.stride;

        OsdCpuComputeStencils(_currentBindState.vertexDesc,
                              srcBuffer, destBuffer,
                              &vertexStencils->GetSizes().at(0),
                              &vertexStencils->GetOffsets().at(0),
                              &vertexStencils->GetControlIndices().at(0),
                              &vertexStencils->GetWeights().at(0),
                              batch.start,
                              batch.end);
    }

    FarStencilTables const * varyingStencils = context->GetVaryingStencilTables();

    if (varyingStencils and _currentBindState.varyingBuffer) {

        OsdVertexBufferDescriptor const & desc = _currentBindState.varyingDesc;

        float const * srcBuffer = _currentBindState.varyingBuffer + desc.offset;

        float * destBuffer = _currentBindState.varyingBuffer + desc.offset +
            varyingStencils->GetNumControlVertices() * desc.stride;

        OsdCpuComputeStencils(_currentBindState.varyingDesc,
                              srcBuffer, destBuffer,
                              &varyingStencils->GetSizes().at(0),
                              &varyingStencils->GetOffsets().at(0),
                              &varyingStencils->GetControlIndices().at(0),
                              &varyingStencils->GetWeights().at(0),
                              batch.start,
                              batch.end);
    }
}


}  // end namespace OPENSUBDIV_VERSION
}  // end namespace OpenSubdiv
