//
// Project Aeloria
//
// This is the ONLY place the WINDOWS_IGNORE_PACKING_MISMATCH define is used.
//
// Purpose:
//   Modern Visual Studio + Windows SDK (10.0.26100+) emits a hard C2338 static_assert
//   in winnt.h when a project uses /Zp1 (StructMemberAlignment=1Byte) and then includes
//   <windows.h>. The macro suppresses that assert for the duration of the Windows headers.
//
//   We deliberately limit the macro's visibility to the absolute minimum (only while
//   the SDK headers are being parsed) so it cannot affect the layout of game structs
//   (UnitTypeClass, etc.) that must remain exactly 1-byte packed for savegames and
//   multiplayer.
//
// Usage:
//   Replace every raw
//       #include <windows.h>
//   (or <WINDOWS.H>, <ddraw.h> when it pulls windows, etc.)
//   with
//       #include "win32_windows.h"
//
//   This file must be the *only* place the macro is ever defined in the entire solution.
//
// See the expert review (May 2026) for the full rationale.
//
#pragma once

#ifndef WINDOWS_IGNORE_PACKING_MISMATCH
#define WINDOWS_IGNORE_PACKING_MISMATCH
#define __WINDOWS_IGNORE_PACKING_MISMATCH_DEFINED_HERE 1
#endif
#include <windows.h>
#if defined(__WINDOWS_IGNORE_PACKING_MISMATCH_DEFINED_HERE)
#undef WINDOWS_IGNORE_PACKING_MISMATCH
#undef __WINDOWS_IGNORE_PACKING_MISMATCH_DEFINED_HERE
#endif
