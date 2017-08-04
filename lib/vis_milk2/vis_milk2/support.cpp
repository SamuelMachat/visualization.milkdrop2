/*
  LICENSE
  -------
Copyright 2005-2013 Nullsoft, Inc.
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

  * Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer. 

  * Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution. 

  * Neither the name of Nullsoft nor the names of its contributors may be used to 
    endorse or promote products derived from this software without specific prior written permission. 
 
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR 
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND 
FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR 
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT 
OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "support.h"
#include "utility.h"
//#include "../Winamp/wa_ipc.h"
#include <DirectXMath.h>
#include <DirectXPackedVector.h>

bool g_bDebugOutput = false;
bool g_bDumpFileCleared = false;

using namespace DirectX;
using namespace DirectX::PackedVector;

//---------------------------------------------------
void PrepareFor3DDrawing(
        DX11Context *pDevice, 
        int viewport_width,
        int viewport_height,
        float fov_in_degrees, 
        float near_clip,
        float far_clip,
        XMVECTOR* pvEye,
        XMVECTOR* pvLookat,
        XMVECTOR* pvUp
    )
{
    // This function sets up DirectX up for 3D rendering.
    // Only call it once per frame, as it is VERY slow.
    // INPUTS:
    //    pDevice           a pointer to the D3D device
    //    viewport_width    the width of the client area of the window
    //    viewport_height   the height of the client area of the window
    //    fov_in_degrees    the field of view, in degrees
    //    near_clip         the distance to the near clip plane; should be > 0!
    //    far_clip          the distance to the far clip plane
    //    eye               the eyepoint coordinates, in world space
    //    lookat            the point toward which the eye is looking, in world space
    //    up                a vector indicating which dir. is up; usually <0,1,0>
    //
    // What this function does NOT do:
    //    1. set the current texture (SetTexture)
    //    2. set up the texture stages for texturing (SetTextureStageState)
    //    3. set the current vertex format (SetVertexShader)
    //    4. set up the world matrix (SetTransform(D3DTS_WORLD, &my_world_matrix))

  {
    pDevice->SetDepth(true);
    pDevice->SetRasterizerState(D3D11_CULL_NONE, D3D11_FILL_SOLID);
    pDevice->SetBlendState(false);
    pDevice->SetSamplerState(0, D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_WRAP);
  }
    // set up render state to some nice defaults:
    /*{
        // some defaults
        pDevice->SetRenderState( D3DRS_ZENABLE, TRUE );
        pDevice->SetRenderState( D3DRS_ZWRITEENABLE, TRUE );
        pDevice->SetRenderState( D3DRS_ZFUNC,     D3DCMP_LESSEQUAL );
        pDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE );
        pDevice->SetRenderState( D3DRS_CLIPPING, TRUE );
        pDevice->SetRenderState( D3DRS_LIGHTING, FALSE );
        pDevice->SetRenderState( D3DRS_COLORVERTEX, TRUE );
        pDevice->SetRenderState( D3DRS_SHADEMODE, D3DSHADE_GOURAUD );
        pDevice->SetRenderState( D3DRS_FILLMODE,  D3DFILL_SOLID );
        pDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );

        // turn fog off
        pDevice->SetRenderState( D3DRS_FOGENABLE, FALSE );
        pDevice->SetRenderState( D3DRS_RANGEFOGENABLE, FALSE );
    
        // turn on high-quality bilinear interpolations
        pDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR); 
        pDevice->SetSamplerState(1, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
        pDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
        pDevice->SetSamplerState(1, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
        pDevice->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
        pDevice->SetSamplerState(1, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
    }*/    

    // set up view & projection matrices (but not the world matrix!)
    {
        // if the window is not square, instead of distorting the scene,
        // clip it so that the longer dimension of the window has the
        // regular FOV, and the shorter dimension has a reduced FOV.
        float fov_x = fov_in_degrees * 3.1415927f/180.0f;
        float fov_y = fov_in_degrees * 3.1415927f/180.0f;
        float aspect = (float)viewport_height / (float)viewport_width;
        if (aspect < 1)
            fov_y *= aspect;
        else
            fov_x /= aspect;
        
        if (near_clip < 0.1f)
            near_clip = 0.1f;
        if (far_clip < near_clip + 1.0f)
            far_clip = near_clip + 1.0f;

        XMMATRIX proj;
        MakeProjectionMatrix(&proj, near_clip, far_clip, fov_x, fov_y);
        pDevice->SetTransform(3 /*D3DTS_PROJECTION*/, &proj);
        
        XMMATRIX view = XMMatrixLookAtLH(*pvEye, *pvLookat, *pvUp);
        //D3DXMatrixLookAtLH(&view, pvEye, pvLookat, pvUp);
        pDevice->SetTransform(2 /*D3DTS_VIEW*/, &view);

        // Optimization note: "You can minimize the number of required calculations 
        // by concatenating your world and view matrices into a world-view matrix 
        // that you set as the world matrix, and then setting the view matrix 
        // to the identity."
        //D3DXMatrixMultiply(&world, &world, &view);                
        //D3DXMatrixIdentity(&view);
    }
}

void PrepareFor2DDrawing(DX11Context *pDevice)
{
    // New 2D drawing area will have x,y coords in the range <-1,-1> .. <1,1>
    //         +--------+ Y=-1
    //         |        |
    //         | screen |             Z=0: front of scene
    //         |        |             Z=1: back of scene
    //         +--------+ Y=1
    //       X=-1      X=1
    // NOTE: After calling this, be sure to then call (at least):
    //  1. SetVertexShader()
    //  2. SetTexture(), if you need it
    // before rendering primitives!
    // Also, be sure your sprites have a z coordinate of 0.

  pDevice->SetDepth(true);
  pDevice->SetRasterizerState(D3D11_CULL_NONE, D3D11_FILL_SOLID);
  pDevice->SetTexture(0, NULL);
  pDevice->SetTexture(1, NULL);
  pDevice->SetSamplerState(0, D3D11_FILTER_MIN_MAG_MIP_POINT, D3D11_TEXTURE_ADDRESS_WRAP);
  pDevice->SetSamplerState(1, D3D11_FILTER_MIN_MAG_MIP_POINT, D3D11_TEXTURE_ADDRESS_WRAP);
  pDevice->SetBlendState(false);
  pDevice->SetShader(1);

  /*
    pDevice->SetRenderState( D3DRS_ZENABLE, TRUE );
    pDevice->SetRenderState( D3DRS_ZWRITEENABLE, TRUE );
    pDevice->SetRenderState( D3DRS_ZFUNC,     D3DCMP_LESSEQUAL );
    pDevice->SetRenderState( D3DRS_SHADEMODE, D3DSHADE_FLAT );
    pDevice->SetRenderState( D3DRS_FILLMODE,  D3DFILL_SOLID );
    pDevice->SetRenderState( D3DRS_FOGENABLE, FALSE );
    pDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE );
    pDevice->SetRenderState( D3DRS_CLIPPING, TRUE ); 
    pDevice->SetRenderState( D3DRS_LIGHTING, FALSE );
    pDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
    pDevice->SetRenderState( D3DRS_LOCALVIEWER, FALSE );
    
    pDevice->SetTexture(0, NULL);
    pDevice->SetTexture(1, NULL);
    pDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);//D3DTEXF_LINEAR);
    pDevice->SetSamplerState(1, D3DSAMP_MAGFILTER, D3DTEXF_POINT);//D3DTEXF_LINEAR);
    pDevice->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
    pDevice->SetTextureStageState(1, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);    
    pDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE );
    pDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
    pDevice->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_CURRENT );
    pDevice->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE );

    pDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1 );
    pDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE );
    pDevice->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE );

    pDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
  */  
    // set up for 2D drawing:
    {
        XMMATRIX Ortho2D = XMMatrixOrthographicLH(2.0f, -2.0f, 0.0f, 1.0f);
        XMMATRIX Identity = XMMatrixIdentity();

        pDevice->SetTransform(3 /*D3DTS_PROJECTION*/, &Ortho2D);
        pDevice->SetTransform(256 /*D3DTS_WORLD*/, &Identity);
        pDevice->SetTransform(2 /*D3DTS_VIEW*/, &Identity);
    }
}

//---------------------------------------------------

void MakeWorldMatrix( XMMATRIX* pOut, 
                      float xpos, float ypos, float zpos, 
                      float sx,   float sy,   float sz, 
                      float pitch, float yaw, float roll)
{
    /*
     * The m_xPos, m_yPos, m_zPos variables contain the model's
     * location in world coordinates.
     * The m_fPitch, m_fYaw, and m_fRoll variables are floats that 
     * contain the model's orientation in terms of pitch, yaw, and roll
     * angles, in radians.
     */

    XMMATRIX MatTemp;
    *pOut = XMMatrixIdentity();

    // 1. first, rotation
    if (pitch || yaw || roll) 
    {
        XMMATRIX MatRot = XMMatrixRotationRollPitchYaw(pitch, yaw, roll);

        //D3DXMatrixRotationX(&MatTemp, pitch);         // Pitch
        //D3DXMatrixMultiply(&MatRot, &MatRot, &MatTemp);
        //D3DXMatrixRotationY(&MatTemp, yaw);           // Yaw
        //D3DXMatrixMultiply(&MatRot, &MatRot, &MatTemp);
        //D3DXMatrixRotationZ(&MatTemp, roll);          // Roll
        //D3DXMatrixMultiply(&MatRot, &MatRot, &MatTemp);
 
        //D3DXMatrixMultiply(pOut, pOut, &MatRot);
        *pOut = XMMatrixMultiply(*pOut, MatRot);
    }

    // 2. then, scaling
    //D3DXMatrixScaling(&MatTemp, sx, sy, sz);
    //D3DXMatrixMultiply(pOut, pOut, &MatTemp);
    MatTemp = XMMatrixScaling(sx, sy, sz);
    *pOut = XMMatrixMultiply(*pOut, MatTemp);

    // 3. last, translation to final world pos.
    //D3DXMatrixTranslation(&MatTemp, xpos, ypos, zpos);
    //D3DXMatrixMultiply(pOut, pOut, &MatTemp);
    MatTemp = XMMatrixTranslation(xpos, ypos, zpos);
    *pOut = XMMatrixMultiply(*pOut, MatTemp);
}

void MakeProjectionMatrix( XMMATRIX* pOut,
                           const float near_plane, // Distance to near clipping plane
                           const float far_plane,  // Distance to far clipping plane
                           const float fov_horiz,  // Horizontal field of view angle, in radians
                           const float fov_vert)   // Vertical field of view angle, in radians
{
    float w = (float)1/tanf(fov_horiz*0.5f);  // 1/tan(x) == cot(x)
    float h = (float)1/tanf(fov_vert*0.5f);   // 1/tan(x) == cot(x)
    float Q = far_plane/(far_plane - near_plane);
 
    *pOut = XMMATRIX(    w, 0.0f,          0.0f, 0.0f,
                      0.0f,    h,          0.0f, 0.0f,
                      0.0f, 0.0f,             Q, 1.0f,
                      0.0f, 0.0f, -Q*near_plane, 0.0f);
    /*pOut->_11 = w;
    pOut->_22 = h;
    pOut->_33 = Q;
    pOut->_43 = -Q*near_plane;
    pOut->_34 = 1;*/
}

int GetDX11TexFormatBitsPerPixel(DXGI_FORMAT fmt)
{
  switch (fmt)
  {
  case DXGI_FORMAT_R32G32B32A32_TYPELESS:
  case DXGI_FORMAT_R32G32B32A32_FLOAT:
  case DXGI_FORMAT_R32G32B32A32_UINT:
  case DXGI_FORMAT_R32G32B32A32_SINT:
    return 128;

  case DXGI_FORMAT_R32G32B32_TYPELESS:
  case DXGI_FORMAT_R32G32B32_FLOAT:
  case DXGI_FORMAT_R32G32B32_UINT:
  case DXGI_FORMAT_R32G32B32_SINT:
    return 96;

  case DXGI_FORMAT_R16G16B16A16_TYPELESS:
  case DXGI_FORMAT_R16G16B16A16_FLOAT:
  case DXGI_FORMAT_R16G16B16A16_UNORM:
  case DXGI_FORMAT_R16G16B16A16_UINT:
  case DXGI_FORMAT_R16G16B16A16_SNORM:
  case DXGI_FORMAT_R16G16B16A16_SINT:
  case DXGI_FORMAT_R32G32_TYPELESS:
  case DXGI_FORMAT_R32G32_FLOAT:
  case DXGI_FORMAT_R32G32_UINT:
  case DXGI_FORMAT_R32G32_SINT:
  case DXGI_FORMAT_R32G8X24_TYPELESS:
  case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
  case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
  case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
    return 64;

  case DXGI_FORMAT_R10G10B10A2_TYPELESS:
  case DXGI_FORMAT_R10G10B10A2_UNORM:
  case DXGI_FORMAT_R10G10B10A2_UINT:
  case DXGI_FORMAT_R11G11B10_FLOAT:
  case DXGI_FORMAT_R8G8B8A8_TYPELESS:
  case DXGI_FORMAT_R8G8B8A8_UNORM:
  case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
  case DXGI_FORMAT_R8G8B8A8_UINT:
  case DXGI_FORMAT_R8G8B8A8_SNORM:
  case DXGI_FORMAT_R8G8B8A8_SINT:
  case DXGI_FORMAT_R16G16_TYPELESS:
  case DXGI_FORMAT_R16G16_FLOAT:
  case DXGI_FORMAT_R16G16_UNORM:
  case DXGI_FORMAT_R16G16_UINT:
  case DXGI_FORMAT_R16G16_SNORM:
  case DXGI_FORMAT_R16G16_SINT:
  case DXGI_FORMAT_R32_TYPELESS:
  case DXGI_FORMAT_D32_FLOAT:
  case DXGI_FORMAT_R32_FLOAT:
  case DXGI_FORMAT_R32_UINT:
  case DXGI_FORMAT_R32_SINT:
  case DXGI_FORMAT_R24G8_TYPELESS:
  case DXGI_FORMAT_D24_UNORM_S8_UINT:
  case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
  case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
  case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
  case DXGI_FORMAT_R8G8_B8G8_UNORM:
  case DXGI_FORMAT_G8R8_G8B8_UNORM:
  case DXGI_FORMAT_B8G8R8A8_UNORM:
  case DXGI_FORMAT_B8G8R8X8_UNORM:
  case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
  case DXGI_FORMAT_B8G8R8A8_TYPELESS:
  case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
  case DXGI_FORMAT_B8G8R8X8_TYPELESS:
  case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
    return 32;

  case DXGI_FORMAT_R8G8_TYPELESS:
  case DXGI_FORMAT_R8G8_UNORM:
  case DXGI_FORMAT_R8G8_UINT:
  case DXGI_FORMAT_R8G8_SNORM:
  case DXGI_FORMAT_R8G8_SINT:
  case DXGI_FORMAT_R16_TYPELESS:
  case DXGI_FORMAT_R16_FLOAT:
  case DXGI_FORMAT_D16_UNORM:
  case DXGI_FORMAT_R16_UNORM:
  case DXGI_FORMAT_R16_UINT:
  case DXGI_FORMAT_R16_SNORM:
  case DXGI_FORMAT_R16_SINT:
  case DXGI_FORMAT_B5G6R5_UNORM:
  case DXGI_FORMAT_B5G5R5A1_UNORM:
  case DXGI_FORMAT_B4G4R4A4_UNORM:
    return 16;

  case DXGI_FORMAT_R8_TYPELESS:
  case DXGI_FORMAT_R8_UNORM:
  case DXGI_FORMAT_R8_UINT:
  case DXGI_FORMAT_R8_SNORM:
  case DXGI_FORMAT_R8_SINT:
  case DXGI_FORMAT_A8_UNORM:
    return 8;

  case DXGI_FORMAT_R1_UNORM:
    return 1;

  case DXGI_FORMAT_BC1_TYPELESS:
  case DXGI_FORMAT_BC1_UNORM:
  case DXGI_FORMAT_BC1_UNORM_SRGB:
  case DXGI_FORMAT_BC4_TYPELESS:
  case DXGI_FORMAT_BC4_UNORM:
  case DXGI_FORMAT_BC4_SNORM:
    return 4;

  case DXGI_FORMAT_BC2_TYPELESS:
  case DXGI_FORMAT_BC2_UNORM:
  case DXGI_FORMAT_BC2_UNORM_SRGB:
  case DXGI_FORMAT_BC3_TYPELESS:
  case DXGI_FORMAT_BC3_UNORM:
  case DXGI_FORMAT_BC3_UNORM_SRGB:
  case DXGI_FORMAT_BC5_TYPELESS:
  case DXGI_FORMAT_BC5_UNORM:
  case DXGI_FORMAT_BC5_SNORM:
  case DXGI_FORMAT_BC6H_TYPELESS:
  case DXGI_FORMAT_BC6H_UF16:
  case DXGI_FORMAT_BC6H_SF16:
  case DXGI_FORMAT_BC7_TYPELESS:
  case DXGI_FORMAT_BC7_UNORM:
  case DXGI_FORMAT_BC7_UNORM_SRGB:
    return 8;

  default:
    return 0;
  }
}