#include "../include/Reflection/Macro.h"

namespace Gleam {

GSTRUCT(Size, "67C7965D-59AD-4506-9DE9-C1902B126DDA", Serializable)
{
    GFIELD("9D34DD6A-2C5D-47A6-8B1C-652C1847B590", Serializable)
    float width = 0.0f;

    GFIELD("C655B9C2-CE7C-4827-8221-0D2EB0584418", Serializable)
    float height = 0.0f;
};

GENUM(WindowFlag, "193C9225-5267-46DD-BAED-9CBE464BB5CE", Serializable)
{
	GITEM(BorderlessFullscreen, "4C1556D5-3768-4253-B326-4914439C5392") = 0,
	GITEM(ExclusiveFullscreen, "1F337950-09AE-4B7C-AACC-9BED99630418") = 1,
	GITEM(MaximizedWindow, "3BE7B62B-CCC5-4377-87EE-765073A2E2B6") = 2,
	GITEM(CustomWindow, "AF98F11E-512E-4ECA-8BEE-FA152AF2B210") = 3
};

GSTRUCT(WindowConfig, "85D3831E-DFF6-4CA8-BFC3-33F624523C52", Serializable)
{
    GFIELD("807E4FDF-31E0-4E28-BDE4-59C3139BBBCC", Serializable)
	WindowFlag windowFlag = WindowFlag::MaximizedWindow;

    GFIELD("3E90C6A1-D720-4DA5-A2C2-1CCE5F2E5C9D", Serializable)
    Size size = {0.0f, 0.0f};

    GFIELD("DD29A72B-B7CD-4FB5-8354-8FC832C16FFE", Serializable)
    unsigned int refreshRate = 0;
};

GSTRUCT(RendererConfig, "A0A57407-A24F-451D-9192-8ACE2E49CFC6", Serializable)
{
    GFIELD("84339532-3BA2-45B6-8CDC-6F9634490219", Serializable)
	bool vsync = true;

    GFIELD("10ADCBAF-D329-4387-8660-875EFC54BEEC", Serializable)
	bool tripleBufferingEnabled = true;
};

GSTRUCT(EngineConfig, "EE9A0D16-3BD0-4C93-9D3B-EB4B43042EF1", Serializable)
{
    GFIELD("2C73BB62-C36E-43FA-85FB-F826186AD7D9", Serializable)
    WindowConfig window;

    GFIELD("971A0BD0-E1FC-49F8-A2EB-B5BAAC7BEDF2", Serializable)
    RendererConfig renderer;
};

namespace Renderer {

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

} // namespace Renderer

} // namespace Gleam