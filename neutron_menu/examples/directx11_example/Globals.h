#pragma once
static const char* aimmodes[]
{
    "memory",
};

static const char* hitboxes[]
{
    "head",
    "neck",
    "body",
    "root",
    "pelvis"
};

static const char* boxmodes[]
{
    "2d",
    "cornered",
    "2d filled",
    "cornered filled",
    "rbga corner filled",
    "filled no box"
};

static const char* linemodes[]
{
    "bottom",
    "top",
    "center",
    "rgba"
};

static const char* aimbonelist[]
{
    "head",
    "neck",
    "body",
    "root",
    "pelvis"

};
struct {
    bool Aimbot;
    bool AutoAimbot;
    bool SilentAimbot;
    float AimbotFOV;
    int HitBoxPos = 0;
    float AimbotSlow;
    float SniperAimbotSlow;
    float FOV;
    bool Crosshair;
    bool WaterMark;
    bool Rainbow;
    bool Silent;
    bool Prediction;
    bool FovChanger;
    int FovValue;
    bool TargetLine;
    int bonepriority;

    int AimbotModePos;
    bool ColorAdjuster;

    int Aimtypes;

    int CrosshairThickness;
    int CrosshairScale;

    bool MemesTest;
    bool TestKek;
    bool Info;
    bool BulletTP;
    bool IsBulletTeleporting;

    bool VisibleCheck;
    bool FPS;
    int AimKey;
    int SpinbotYawMode;
    int SnaplineStartPoint;

    struct {


        bool AimbotFOV;
        bool Boxes;
        bool Visuals;
        bool Skeletons;
        bool PlayerLines;
        bool PlayerNames;
        bool PlayerWeapons;
        bool PlayerAmmo;
        bool LLamas;
        bool Memes;
        bool Radar;
        bool TestChams;

        float PlayerNameVisibleColor[4];
        float PlayerNameNotVisibleColor[4];
        float PlayerNameColor[4];
        float BoxVisibleColor[4];
        float BoxNotVisibleColor[4];
        float SnaplineVisibleColor[4];
        float SnaplineNotVisibleColor[4];
        float SkeletonVisibleColor[4];
        float SkeletonNotVisibleColor[4];
        float FovColor[4];
        float TargetLineColor[4];
        bool debug;
        bool debug2;
        bool Ammo;
        bool AmmoBox;
        bool Containers;
        bool Weapons;
        bool Vehicles;
        bool SupplyDrops;
    } ESP;
}Settings;
namespace Global
{
    bool dx_init = false;
    ImVec4 RGB;
    int my_image_width = 0, my_image_height = 0, my_bg_width = 0, my_bg_height = 0;
    ID3D11ShaderResourceView* my_texture = NULL;
    ID3D11ShaderResourceView* bg_texture = NULL;
    
}