# CPU Temperature Calibration System (TjMax Calibration)

## Problem

Intel hybrid architecture CPUs (Alder Lake and later) contain multiple core types:
- P-Core (Performance Core)
- E-Core (Efficient Core)
- LP-Core (Low-Power Efficient Core)

Each core domain has its own TjMax (maximum junction temperature), used to convert
MSR 0x19C deltaT to actual temperature:
  Temperature = TjMax - deltaT

Issue: MSR 0x1A2 (IA32_TEMPERATURE_TARGET) returns the same TjMax value for all
core domains on some hybrid CPUs, but the actual E-Core TjMax may be lower.

Measured data (Intel Core Ultra 5 338H, Panther Lake):
- MSR 0x1A2 reports: TjMax = 100 (all cores)
- E-Core actual TjMax: ~93 (derived from HWMonitor comparison)
- Deviation: 7 degrees C

## Solution

### Approach 1: CPU Model Database (Primary)

Maintain a TjMax mapping table for known hybrid CPUs:
  { family, model, P_TjMax, E_TjMax, LP_TjMax, name }

- TjMax=0 means use MSR 0x1A2 default, no correction needed
- CPUID 0x1A detects core type (P/E/LP) for each logical processor
- When database matches, use database TjMax instead of MSR 0x1A2

### Approach 2: Package Temperature Cross-Validation (Fallback)

For hybrid CPUs not in the database:
1. Read average temperature for P-Cores and E-Cores
2. If E-Core avg > P-Core avg + 3 degrees C, suspect E-Core TjMax is too high
3. Auto-correct: newTjMax = 100 - (E_avg - P_avg)

Principle: At idle, P-Cores (performance) are typically warmer than E-Cores (efficiency).
If E-Core readings exceed P-Core readings, the TjMax is likely wrong.

Limitation: Correction accuracy is limited, serves as emergency fallback for unknown CPUs.

## Modified Files

TrayS/PawnIo.cpp:
- Added CpuTjMaxEntry struct and g_cpuTjDb[] database
- Added PawnIo_InitTjCalibration(): CPU model detection + database lookup + core type ID
- Added PawnIo_CrossValidateTjMax(): Package temperature cross-validation
- Modified PawnIo_GetCpuTemp(): apply calibrated TjMax
- Modified PawnIo_GetCpuTempMax(): invoke cross-validation

## Debug Logging (Debug build, view with DebugView)

[TJMAX-DB]      Database match result
[TJMAX-CROSS]   Cross-validation process and result
[TDBG] Core=X TjMax override: Y -> Z   TjMax override applied

## Adding New CPUs to Database

1. Run Debug build, collect [TDBG] logs
2. Compare with HWMonitor temperatures, calculate actual TjMax: actual = HWMonitor + deltaT
3. Add new entry to g_cpuTjDb[]
4. Recompile and test

## Known Database Entries

CPU              Family  Model  P_TjMax  E_TjMax  LP_TjMax  Status
Panther Lake     6       0xCC   0(100)   93       0(100)    Verified
Arrow Lake-S     6       0xC5   0        94       0         Pending
Arrow Lake-P     6       0xC6   0        94       0         Pending