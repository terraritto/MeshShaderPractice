#include "Util.h"
#include <filesystem>

DXGI_FORMAT GetNoSRGBFormat(const DXGI_FORMAT value)
{
    DXGI_FORMAT format = value;

    switch (value)
    {
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB: format = DXGI_FORMAT_R8G8B8A8_UNORM; break;
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB: format = DXGI_FORMAT_B8G8R8A8_UNORM; break;
    case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB: format = DXGI_FORMAT_B8G8R8X8_UNORM; break;
    case DXGI_FORMAT_BC1_UNORM_SRGB: format = DXGI_FORMAT_BC1_UNORM; break;
    case DXGI_FORMAT_BC2_UNORM_SRGB: format = DXGI_FORMAT_BC2_UNORM; break;
    case DXGI_FORMAT_BC3_UNORM_SRGB: format = DXGI_FORMAT_BC3_UNORM; break;
    case DXGI_FORMAT_BC7_UNORM_SRGB: format = DXGI_FORMAT_BC7_UNORM; break;
    default: break;
    }

    return format;
}

DXGI_FORMAT GetSRGBFormat(const DXGI_FORMAT value)
{
    DXGI_FORMAT format = value;

    switch (value)
    {
    case DXGI_FORMAT_R8G8B8A8_UNORM: format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; break;
    case DXGI_FORMAT_B8G8R8A8_UNORM: format = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB; break;
    case DXGI_FORMAT_B8G8R8X8_UNORM: format = DXGI_FORMAT_B8G8R8X8_UNORM_SRGB; break;
    case DXGI_FORMAT_BC1_UNORM: format = DXGI_FORMAT_BC1_UNORM_SRGB; break;
    case DXGI_FORMAT_BC2_UNORM: format = DXGI_FORMAT_BC2_UNORM_SRGB; break;
    case DXGI_FORMAT_BC3_UNORM: format = DXGI_FORMAT_BC3_UNORM_SRGB; break;
    case DXGI_FORMAT_BC7_UNORM: format = DXGI_FORMAT_BC7_UNORM_SRGB; break;
    default: break;
    }

    return format;
}

DXGI_FORMAT GetResourceFormat(const DXGI_FORMAT value, bool isStencil)
{
    DXGI_FORMAT result = value;

    switch (value)
    {
    case DXGI_FORMAT_D16_UNORM: result = DXGI_FORMAT_R16_UNORM; break;
    case DXGI_FORMAT_D32_FLOAT: result = DXGI_FORMAT_R32_FLOAT; break;
    case DXGI_FORMAT_D24_UNORM_S8_UINT: result = isStencil ? DXGI_FORMAT_R24_UNORM_X8_TYPELESS : DXGI_FORMAT_X24_TYPELESS_G8_UINT; break;
    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT: result = isStencil ? DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS : DXGI_FORMAT_X32_TYPELESS_G8X24_UINT; break;
    default: break;
    }

    return result;
}

bool IsSRGBFormat(DXGI_FORMAT value)
{
    bool result = false;
    
    switch (value)
    {
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB: result = true; break;
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB: result = true; break;
    case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB: result = true; break;
    case DXGI_FORMAT_BC1_UNORM_SRGB: result = true; break;
    case DXGI_FORMAT_BC2_UNORM_SRGB: result = true; break;
    case DXGI_FORMAT_BC3_UNORM_SRGB: result = true; break;
    case DXGI_FORMAT_BC7_UNORM_SRGB: result = true; break;
    default: break;
    }

    return result;
}
