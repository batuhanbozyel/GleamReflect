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
    GITEM(Ortho, "8A1A6FA3-4FD8-4FEB-9A60-0944996B5A01"),
    GITEM(Perspective, "8A1A6FA3-4FD8-4FEB-9A60-0944996B5A02")
};

GSTRUCT(Camera, "33D48E5D-6A9F-4D11-8A55-82F5C0EECECE", EntityComponent, Serializable)
{
    // Perspective projection properties
    GFIELD("33D48E5D-6A9F-4D11-8A55-82F5C0EEA001", Serializable)
    float fov = 60.0f;
    
    // Orthographic projection properties
    GFIELD("33D48E5D-6A9F-4D11-8A55-82F5C0EEA002", Serializable)
    float orthographicSize = 5.0f;
    
    // Common properties
    GFIELD("33D48E5D-6A9F-4D11-8A55-82F5C0EEA003", Serializable)
    float aspectRatio = 1.0f;
    
    GFIELD("33D48E5D-6A9F-4D11-8A55-82F5C0EEA004", Serializable)
    float nearPlane = 0.1f;
    
    GFIELD("33D48E5D-6A9F-4D11-8A55-82F5C0EEA005", Serializable)
    float farPlane = 1000.0f;
    
    GFIELD("33D48E5D-6A9F-4D11-8A55-82F5C0EEA006", Serializable)
    ProjectionType projectionType = ProjectionType::Perspective;
    
    // Post-process settings
    GFIELD("33D48E5D-6A9F-4D11-8A55-82F5C0EEA007", Serializable)
    ColorGradingSettings colorGrading = {};
};

} // namespace Gleam

int main()
{
    return 0;
}