#include "stdafx.h"
#pragma hdrstop

#include "Blender_Vertex_aref.h"

#if RENDER != R_R1
#error "This blender can't be used in this renderer generation"
#endif

CBlender_Vertex_aref::CBlender_Vertex_aref()
{
    description.CLS = B_VERT_AREF;
    description.version = 1;
    oAREF.value = 32;
    oAREF.min = 0;
    oAREF.max = 255;
    oBlend.value = FALSE;
}

LPCSTR CBlender_Vertex_aref::getComment()
{
    return "LEVEL: diffuse*base.aref";
}

void CBlender_Vertex_aref::Save(IWriter& fs)
{
    IBlender::Save(fs);

    xrPWRITE_PROP(fs, "Alpha ref", xrPID_INTEGER, oAREF);
    xrPWRITE_PROP(fs, "Alpha-blend", xrPID_BOOL, oBlend);
}

void CBlender_Vertex_aref::Load(IReader& fs, u16 version)
{
    IBlender::Load(fs, version);

    xrPREAD_PROP(fs, xrPID_INTEGER, oAREF);

    switch (version)
    {
    case 0:
        oBlend.value = FALSE;
        break;

    default:
        xrPREAD_PROP(fs, xrPID_BOOL, oBlend);
        break;
    }
}

void CBlender_Vertex_aref::Compile(CBlender_Compile& C)
{
    IBlender::Compile(C);

    if (C.bFFP)
        CompileFFP(C);
    else
        CompileProgrammable(C);
}

void CBlender_Vertex_aref::CompileFFP(CBlender_Compile& C) const
{
    if (ps_r1_ffp_lighting_mode == R1_FFP_LIGHTING_CONSTANT)
    {
        C.PassBegin();
        {
            const D3DBLEND blend_src = oBlend.value ? D3DBLEND_SRCALPHA : D3DBLEND_ONE;
            const D3DBLEND blend_dst = oBlend.value ? D3DBLEND_INVSRCALPHA : D3DBLEND_ZERO;

            //C.PassSET_Blend(true, blend_src, blend_dst, true, oAREF.value);
            C.PassSET_LightFog(true, true);

            // Stage1 - Base texture
            C.StageBegin();
            C.StageSET_Color(D3DTA_TEXTURE, D3DTOP_MODULATE, D3DTA_DIFFUSE);
            C.StageSET_Alpha(D3DTA_TEXTURE, D3DTOP_MODULATE, D3DTA_DIFFUSE);
            C.Stage_Texture(oT_Name);
            C.Stage_Matrix(oT_xform, 0);
            C.Stage_Constant("$null");
            C.StageEnd();
        }
        C.PassEnd();
    }
    else
    {
        switch (C.iElement)
        {
        case SE_R1_NORMAL_HQ:
        case SE_R1_NORMAL_LQ:
        {
            // Level view
            C.PassBegin();
            {
                C.PassSET_ZB(TRUE, TRUE);
                if (oBlend.value)
                    C.PassSET_Blend(TRUE, D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA, TRUE, oAREF.value);
                else
                    C.PassSET_Blend(TRUE, D3DBLEND_ONE, D3DBLEND_ZERO, TRUE, oAREF.value);
                C.PassSET_LightFog(ps_r1_ffp_lighting_mode == R1_FFP_LIGHTING_CONSTANT, TRUE);

                // Stage1 - Base texture
                C.StageBegin();
                C.StageSET_Color(D3DTA_TEXTURE, D3DTOP_MODULATE2X, D3DTA_DIFFUSE);
                C.StageSET_Alpha(D3DTA_TEXTURE, D3DTOP_MODULATE2X, D3DTA_DIFFUSE);
                C.Stage_Texture(oT_Name);
                C.Stage_Matrix(oT_xform, 0);
                C.Stage_Constant("$null");
                C.StageEnd();
            }
            C.PassEnd();
            break;
        }

        case SE_R1_LMODELS:
        {
            // Lighting only
            C.PassBegin();
            {
                C.PassSET_ZB(TRUE, TRUE);
                if (oBlend.value)
                    C.PassSET_Blend(TRUE, D3DBLEND_SRCALPHA, D3DBLEND_INVSRCALPHA, TRUE, oAREF.value);
                else
                    C.PassSET_Blend(TRUE, D3DBLEND_ONE, D3DBLEND_ZERO, TRUE, oAREF.value);
                C.PassSET_LightFog(FALSE, FALSE);

                // Stage1 - Base texture
                C.StageBegin();
                C.StageSET_Color(D3DTA_TEXTURE, D3DTOP_SELECTARG2, D3DTA_DIFFUSE);
                C.StageSET_Alpha(D3DTA_TEXTURE, D3DTOP_SELECTARG1, D3DTA_DIFFUSE);
                C.Stage_Texture(oT_Name);
                C.Stage_Matrix(oT_xform, 0);
                C.Stage_Constant("$null");
                C.StageEnd();
            }
            C.PassEnd();
            break;
        }

        default:
            break;
        } // switch (C.iElement)
    }
}

void CBlender_Vertex_aref::CompileProgrammable(CBlender_Compile& C) const
{
    const D3DBLEND blend_src = oBlend.value ? D3DBLEND_SRCALPHA : D3DBLEND_ONE;
    const D3DBLEND blend_dst = oBlend.value ? D3DBLEND_INVSRCALPHA : D3DBLEND_ZERO;

    switch (C.iElement)
    {
    case SE_R1_NORMAL_HQ:
        // Level view
        C.PassBegin();
        {
            cpcstr tsv_hq = C.bDetail_Diffuse ? "vert_dt" : "vert";
            cpcstr tsp_hq = C.bDetail_Diffuse ? "vert_dt" : "vert";

            C.PassSET_Shaders(tsv_hq, tsp_hq);

            C.PassSET_LightFog(false, true);
            C.PassSET_ablend_mode(true, blend_src, blend_dst);
            C.PassSET_ablend_aref(true, oAREF.value);

            C.SampledImage("s_base", "s_base", C.L_textures[0]);
            if (C.bDetail_Diffuse)
            {
                C.SampledImage("s_detail", "s_detail", C.detail_texture);
            }
        }
        C.PassEnd();
        break;

    case SE_R1_NORMAL_LQ:
        // Level view
        C.PassBegin();
        {
            C.PassSET_Shaders("vert", "vert");

            C.PassSET_LightFog(false, true);
            C.PassSET_ablend_mode(true, blend_src, blend_dst);
            C.PassSET_ablend_aref(true, oAREF.value);

            C.SampledImage("s_base", "s_base", C.L_textures[0]);
        }
        C.PassEnd();
        break;

    case SE_R1_LPOINT:
        C.PassBegin();
        {
            C.PassSET_Shaders("vert_point", "add_point");

            C.PassSET_ZB(true, false);
            C.PassSET_ablend_mode(true, D3DBLEND_ONE, D3DBLEND_ONE);
            C.PassSET_ablend_aref(true, oAREF.value);

            C.SampledImage("s_base", "s_base", C.L_textures[0]);
            C.SampledImage("smp_rtlinear", "s_lmap", TEX_POINT_ATT);
            C.SampledImage("smp_rtlinear", "s_att", TEX_POINT_ATT);;
            if (C.bDetail_Diffuse)
            {
                C.SampledImage("s_detail", "s_detail", C.detail_texture);
            }
        }
        C.PassEnd();
        break;

    case SE_R1_LSPOT:
        C.PassBegin();
        {
            C.PassSET_Shaders("vert_spot", "add_spot");

            C.PassSET_ZB(true, false);
            C.PassSET_ablend_mode(true, D3DBLEND_ONE, D3DBLEND_ONE);
            C.PassSET_ablend_aref(true, oAREF.value);

            C.SampledImage("s_base", "s_base", C.L_textures[0]);
            u32 stage = C.SampledImage("smp_rtlinear", "s_lmap", "internal" DELIMITER "internal_light_att");
            {
                C.i_Projective(stage, true);
            }
            C.SampledImage("smp_rtlinear", "s_att", TEX_SPOT_ATT);
            if (C.bDetail_Diffuse)
            {
                C.SampledImage("s_detail", "s_detail", C.detail_texture);
            }
        }
        C.PassEnd();
        break;

    case SE_R1_LMODELS:
        C.PassBegin();
        {
            C.PassSET_Shaders("vert_l", "vert_l");

            C.SampledImage("s_base", "s_base", C.L_textures[0]);
        }
        C.PassEnd();
        break;

    default:
        break;
    } // switch (C.iElement)
}
