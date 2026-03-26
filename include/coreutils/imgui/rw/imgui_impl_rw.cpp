#include <assert.h>
#include <d3d9.h>
#include <imgui.h>
#include <plugin.h>
#include <rpworld.h>
#include <rwcore.h>
#include <game_sa/common.h>

using namespace plugin;

static RwTexture *g_FontTexture = nullptr;
static RwIm2DVertex *g_vertbuf = nullptr;
static int g_vertbufSize = 0;

//   IDirect3DDevice9* device = *(IDirect3DDevice9**)0xC97C28;  // for GTA:SA

void ImGui_ImplRW_RenderDrawData(ImDrawData *draw_data)
{
    ImGuiIO &io = ImGui::GetIO();
    if (io.DisplaySize.x <= 0.0f || io.DisplaySize.y <= 0.0f)
        return;

    if (g_vertbuf == nullptr || g_vertbufSize < draw_data->TotalVtxCount)
    {
        if (g_vertbuf)
            RwFree(g_vertbuf);
        g_vertbufSize = draw_data->TotalVtxCount + 5000;
        g_vertbuf = (RwIm2DVertex *)RwMalloc(sizeof(RwIm2DVertex) * g_vertbufSize, 0);
    }

    float xoff = 0.0f, yoff = 0.0f;
#ifdef RWHALFPIXEL
    xoff = -0.5f;
    yoff = 0.5f;
#endif

    RwCamera *cam = (RwCamera *)RwCameraGetCurrentCamera();
    RwIm2DVertex *vtx_dst = g_vertbuf;
    float recipZ = 1.0f / RwCameraGetNearClipPlane(cam);

    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList *cmd_list = draw_data->CmdLists[n];
        const ImDrawVert *vtx_src = cmd_list->VtxBuffer.Data;
        for (int i = 0; i < cmd_list->VtxBuffer.Size; i++)
        {
            RwIm2DVertexSetScreenX(&vtx_dst[i], vtx_src[i].pos.x + xoff);
            RwIm2DVertexSetScreenY(&vtx_dst[i], vtx_src[i].pos.y + yoff);
            RwIm2DVertexSetScreenZ(&vtx_dst[i], RwIm2DGetNearScreenZ());
            RwIm2DVertexSetCameraZ(&vtx_dst[i], RwCameraGetNearClipPlane(cam));
            RwIm2DVertexSetRecipCameraZ(&vtx_dst[i], recipZ);

            unsigned int col = vtx_src[i].col;
            float alpha = ((col >> 24) & 0xFF) / 255.0f;
            RwIm2DVertexSetIntRGBA(&vtx_dst[i], (int)((col & 0xFF) * alpha), (int)(((col >> 8) & 0xFF) * alpha),
                                   (int)(((col >> 16) & 0xFF) * alpha), (col >> 24) & 0xFF);

            RwIm2DVertexSetU(&vtx_dst[i], vtx_src[i].uv.x, recipZ);
            RwIm2DVertexSetV(&vtx_dst[i], vtx_src[i].uv.y, recipZ);
        }
        vtx_dst += cmd_list->VtxBuffer.Size;
    }

    // Save RW render states
    void *tex;
    int vertexAlpha, srcBlend, dstBlend, ztest, addrU, addrV, filter, cullmode, shadeMode;
    RwRenderStateGet(rwRENDERSTATEVERTEXALPHAENABLE, &vertexAlpha);
    RwRenderStateGet(rwRENDERSTATESRCBLEND, &srcBlend);
    RwRenderStateGet(rwRENDERSTATEDESTBLEND, &dstBlend);
    RwRenderStateGet(rwRENDERSTATEZTESTENABLE, &ztest);
    RwRenderStateGet(rwRENDERSTATETEXTURERASTER, &tex);
    RwRenderStateGet(rwRENDERSTATETEXTUREADDRESSU, &addrU);
    RwRenderStateGet(rwRENDERSTATETEXTUREADDRESSV, &addrV);
    RwRenderStateGet(rwRENDERSTATETEXTUREFILTER, &filter);
    RwRenderStateGet(rwRENDERSTATECULLMODE, &cullmode);
    RwRenderStateGet(rwRENDERSTATESHADEMODE, &shadeMode);

    // Setup render state for ImGui
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void *)TRUE);
    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void *)rwBLENDSRCALPHA);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void *)rwBLENDINVSRCALPHA);
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void *)FALSE);
    RwRenderStateSet(rwRENDERSTATECULLMODE, (void *)rwCULLMODECULLNONE);
    RwRenderStateSet(rwRENDERSTATESHADEMODE, (void *)rwSHADEMODEGOURAUD);

    // Get D3D device for scissor
    IDirect3DDevice9 *d3ddev = (IDirect3DDevice9 *)GetD3DDevice();
    RECT oldScissor;
    d3ddev->GetScissorRect(&oldScissor);
    BOOL oldScissorEnable;
    d3ddev->GetRenderState(D3DRS_SCISSORTESTENABLE, (DWORD *)&oldScissorEnable);

    int vtx_offset = 0;
    for (int n = 0; n < draw_data->CmdListsCount; n++)
    {
        const ImDrawList *cmd_list = draw_data->CmdLists[n];
        int idx_offset = 0;
        for (int i = 0; i < cmd_list->CmdBuffer.Size; i++)
        {
            const ImDrawCmd *pcmd = &cmd_list->CmdBuffer[i];
            if (pcmd->UserCallback)
            {
                pcmd->UserCallback(cmd_list, pcmd);
            }
            else
            {
                // Setup scissor rect
                RECT r;
                r.left = (LONG)pcmd->ClipRect.x;
                r.top = (LONG)pcmd->ClipRect.y;
                r.right = (LONG)pcmd->ClipRect.z;
                r.bottom = (LONG)pcmd->ClipRect.w;
                d3ddev->SetRenderState(D3DRS_SCISSORTESTENABLE, TRUE);
                d3ddev->SetScissorRect(&r);

                // Set texture
                RwTexture *tex2 = (RwTexture *)pcmd->GetTexID();
                if (tex2 && tex2->raster)
                    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, tex2->raster);

                // Render
                RwIm2DRenderIndexedPrimitive(rwPRIMTYPETRILIST, g_vertbuf + vtx_offset, cmd_list->VtxBuffer.Size,
                                             cmd_list->IdxBuffer.Data + idx_offset, pcmd->ElemCount);
            }
            idx_offset += pcmd->ElemCount;
        }
        vtx_offset += cmd_list->VtxBuffer.Size;
    }

    // Restore scissor state
    d3ddev->SetRenderState(D3DRS_SCISSORTESTENABLE, oldScissorEnable);
    d3ddev->SetScissorRect(&oldScissor);

    // Restore RW render states
    RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void *)vertexAlpha);
    RwRenderStateSet(rwRENDERSTATESRCBLEND, (void *)srcBlend);
    RwRenderStateSet(rwRENDERSTATEDESTBLEND, (void *)dstBlend);
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void *)ztest);
    RwRenderStateSet(rwRENDERSTATETEXTURERASTER, tex);
    RwRenderStateSet(rwRENDERSTATETEXTUREADDRESSU, (void *)addrU);
    RwRenderStateSet(rwRENDERSTATETEXTUREADDRESSV, (void *)addrV);
    RwRenderStateSet(rwRENDERSTATETEXTUREFILTER, (void *)filter);
    RwRenderStateSet(rwRENDERSTATECULLMODE, (void *)cullmode);
    RwRenderStateSet(rwRENDERSTATESHADEMODE, (void *)shadeMode);
}

bool ImGui_ImplRW_Init()
{
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    return true;
}

void ImGui_ImplRW_Shutdown()
{
}

static bool ImGui_ImplRW_CreateFontsTexture()
{
    ImGuiIO &io = ImGui::GetIO();
    unsigned char *pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height, NULL);

    RwImage *image = RwImageCreate(width, height, 32);
    RwImageAllocatePixels(image);

    for (int y = 0; y < height; y++)
    {
        memcpy(image->cpPixels + image->stride * y, pixels + width * 4 * y, width * 4);
    }

    RwRaster *raster = RwRasterCreate(width, height, 32, rwRASTERTYPETEXTURE);
    RwRasterSetFromImage(raster, image);

    g_FontTexture = RwTextureCreate(raster);
    RwTextureSetFilterMode(g_FontTexture, rwFILTERLINEAR);

    RwImageDestroy(image);

    io.Fonts->TexID = (ImTextureID)g_FontTexture;
    return true;
}

bool ImGui_ImplRW_CreateDeviceObjects()
{
    return ImGui_ImplRW_CreateFontsTexture();
}

void ImGui_ImplRW_NewFrame()
{
    if (!g_FontTexture)
    {
        ImGui_ImplRW_CreateDeviceObjects();
    }

    ImGuiIO &io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)screen::GetScreenWidth(), (float)screen::GetScreenHeight());
    io.DeltaTime = std::max(1 / 60.0f, CTimer::ms_fTimeStep / 1000.0f); // fallback to 60 FPS

    io.KeyCtrl = ImGui::IsKeyPressed(ImGuiKey_LeftCtrl) || ImGui::IsKeyPressed(ImGuiKey_RightCtrl);
    io.KeyShift = ImGui::IsKeyPressed(ImGuiKey_LeftShift) || ImGui::IsKeyPressed(ImGuiKey_RightShift);
    io.KeyAlt = ImGui::IsKeyPressed(ImGuiKey_LeftAlt) || ImGui::IsKeyPressed(ImGuiKey_RightAlt);
    io.KeySuper = false;
}
