#include "Macro.h"

namespace Gleam {

GENUM(Tonemapping, "6B993432-E807-444D-AB3D-8B6F6BD8F84D", Serializable)
{
    GITEM(None, "A1993432-E807-444D-AB3D-8B6F6BD8F84D"),
    GITEM(ACES, "B2993432-E807-444D-AB3D-8B6F6BD8F84D"),
    GITEM(Neutral, "C3993432-E807-444D-AB3D-8B6F6BD8F84D"),
    GITEM(Filmic, "D4993432-E807-444D-AB3D-8B6F6BD8F84D"),
    GITEM(AgX, "E5993432-E807-444D-AB3D-8B6F6BD8F84D")
};

GSTRUCT(ColorGradingSettings, "CD1CEEDD-2481-4000-B165-BCC6A1953E00", Serializable)
{
    GFIELD("F6993432-E807-444D-AB3D-8B6F6BD8F84D", Serializable)
    Tonemapping tonemapping = Tonemapping::ACES;
};

GENUM(ProjectionType, "8A1A6FA3-4FD8-4FEB-9A60-0944996B5ABF", Serializable)
{
    GITEM(Ortho, "E1F5CE21-8EB0-4F7A-946A-CABE10F5EB47"),
    GITEM(Perspective, "802333FA-CD4B-400C-84D1-7337792052C9")
};

GSTRUCT(Camera, "33D48E5D-6A9F-4D11-8A55-82F5C0EECECE", EntityComponent, Serializable)
{
    // Perspective projection properties
    GFIELD("2DF051A6-5FB3-4002-B663-091754549506", Serializable)
    float fov = 60.0f;
    
    // Orthographic projection properties
    GFIELD("2DA0EDF0-C0A6-4E64-94AE-378B133A227B", Serializable)
    float orthographicSize = 5.0f;
    
    // Common properties
    GFIELD("6C7BBB62-642C-45A0-8ED3-6B3DDC803657", Serializable)
    float aspectRatio = 1.0f;
    
    GFIELD("3C8FBDD0-24C3-4785-9521-5CED2B1CCC13", Serializable)
    float nearPlane = 0.1f;
    
    GFIELD("983DC228-93FE-4F78-89B7-04786E8E6730", Serializable)
    float farPlane = 1000.0f;
    
    GFIELD("B1872ADE-DF46-4A4E-ACA7-7D2204BAEB1D", Serializable)
    ProjectionType projectionType = ProjectionType::Perspective;
    
    // Post-process settings
    GFIELD("DB068ED3-C0F5-49AA-8D5A-C990182E6663", Serializable)
    ColorGradingSettings colorGrading = {};
};

} // namespace Gleam

int main()
{
    return 0;
}