// HardKick factory presets
#pragma once
#include "KickEngine.h"

namespace hk
{

inline int numPresets() { return 7; }

inline const char* presetName (int i)
{
    static const char* names[] = { "Init Hardstyle", "Raw Screech", "Euphoric", "Zaag",
                                   "Short Gated", "Uptempo Crunch", "Clean Punch" };
    return (i >= 0 && i < numPresets()) ? names[i] : "Init Hardstyle";
}

inline KickParams presetParams (int i)
{
    KickParams p; // defaults = Init Hardstyle
    switch (i)
    {
        case 1: // Raw Screech
            p.clickLevel = 0.55f; p.clickTone = 6000.f; p.clickDecay = 4.f;
            p.startPitch = 3000.f; p.sweepTime = 45.f; p.sweepCurve = 0.65f; p.punchDrive = 10.f; p.punchDecay = 70.f;
            p.tailShape = 0.35f; p.tailStart = 28.f; p.tailAttack = 12.f; p.tailLength = 320.f; p.tailRelease = 140.f;
            p.pitchDrop = 3.f;
            p.dist1Type = DistCrunch; p.drive1 = 34.f; p.dist2Type = DistHard; p.drive2 = 9.f;
            p.cutoff = 5000.f; p.resonance = 0.45f; p.filterEnvAmt = 1.0f; p.filterDecay = 200.f;
            p.masterDrive = 4.f;
            break;

        case 2: // Euphoric
            p.clickLevel = 0.35f; p.clickTone = 3500.f; p.clickDecay = 7.f;
            p.startPitch = 1800.f; p.sweepTime = 65.f; p.sweepCurve = 0.5f; p.punchDrive = 5.f; p.punchDecay = 120.f;
            p.tailShape = 0.1f; p.tailStart = 45.f; p.tailAttack = 30.f; p.tailLength = 300.f; p.tailRelease = 200.f;
            p.pitchDrop = 1.5f;
            p.dist1Type = DistSoft; p.drive1 = 22.f; p.dist2Type = DistSoft; p.drive2 = 6.f;
            p.cutoff = 2200.f; p.resonance = 0.1f; p.filterEnvAmt = 1.5f; p.filterDecay = 150.f;
            break;

        case 3: // Zaag
            p.startPitch = 2600.f; p.sweepTime = 50.f; p.punchDrive = 8.f;
            p.tailShape = 1.0f; p.tailStart = 30.f; p.tailAttack = 15.f; p.tailLength = 280.f; p.tailRelease = 180.f;
            p.pitchDrop = 5.f;
            p.dist1Type = DistFold; p.drive1 = 12.f; p.dist2Type = DistHard; p.drive2 = 10.f;
            p.cutoff = 4500.f; p.resonance = 0.3f; p.filterEnvAmt = 2.0f; p.filterDecay = 250.f;
            break;

        case 4: // Short Gated
            p.startPitch = 2400.f; p.sweepTime = 40.f; p.punchDecay = 70.f;
            p.tailStart = 25.f; p.tailAttack = 10.f; p.tailLength = 90.f; p.tailRelease = 60.f;
            p.pitchDrop = 1.f;
            p.dist1Type = DistHard; p.drive1 = 26.f; p.dist2Type = DistSoft; p.drive2 = 4.f;
            p.cutoff = 3000.f;
            break;

        case 5: // Uptempo Crunch
            p.clickLevel = 0.6f; p.clickTone = 7000.f; p.clickDecay = 3.f;
            p.startPitch = 3500.f; p.sweepTime = 35.f; p.sweepCurve = 0.7f; p.punchDrive = 12.f; p.punchDecay = 55.f;
            p.tailShape = 0.5f; p.tailStart = 20.f; p.tailAttack = 8.f; p.tailLength = 140.f; p.tailRelease = 80.f;
            p.pitchDrop = 2.5f;
            p.dist1Type = DistCrunch; p.drive1 = 40.f; p.dist2Type = DistFold; p.drive2 = 3.f;
            p.cutoff = 6000.f; p.resonance = 0.35f; p.filterEnvAmt = 0.5f;
            p.masterDrive = 6.f;
            break;

        case 6: // Clean Punch
            p.clickLevel = 0.3f; p.punchDrive = 2.f; p.punchDecay = 150.f;
            p.tailShape = 0.0f; p.tailStart = 60.f; p.tailAttack = 40.f; p.tailLength = 200.f; p.tailRelease = 250.f;
            p.pitchDrop = 0.f;
            p.dist1Type = DistSoft; p.drive1 = 6.f; p.dist2Type = DistOff; p.drive2 = 0.f;
            p.cutoff = 1500.f; p.filterEnvAmt = 0.f;
            p.masterDrive = 0.f;
            break;

        default: break;
    }
    return p;
}

} // namespace hk
