//
// Copyright 2020 Electronic Arts Inc.
//
// TiberianDawn.DLL and RedAlert.dll and corresponding source code is free 
// software: you can redistribute it and/or modify it under the terms of 
// the GNU General Public License as published by the Free Software Foundation, 
// either version 3 of the License, or (at your option) any later version.

// TiberianDawn.DLL and RedAlert.dll and corresponding source code is distributed 
// in the hope that it will be useful, but with permitted additional restrictions 
// under Section 7 of the GPL. See the GNU General Public License in LICENSE.TXT 
// distributed with this program. You should have received a copy of the 
// GNU General Public License along with permitted additional restrictions 
// with this program. If not, see https://github.com/electronicarts/CnC_Remastered_Collection

/* $Header: /CounterStrike/OBJECT.H 1     3/03/97 10:25a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer                                            *
 *                                                                                             *
 *                    File Name : OBJECT.H                                                     *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic                                                *
 *                                                                                             *
 *                   Start Date : April 29, 1994                                               *
 *                                                                                             *
 *                  Last Update : April 29, 1994   [JLB]                                       *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef OBJECT_H
#define OBJECT_H

#include	"abstract.h"
#include	<map>

class ObjectClass;
class TechnoClass;
class ObjectTypeClass;
class HouseClass;
class BuildingClass;
class RadioClass;
class TriggerClass;


/**********************************************************************
**	Every game object (that can exist on the map) is ultimately derived from this object
**	class. It holds the common information between all objects. This is primarily the
**	object unique ID number and its location in the world. All common operations between
**	game objects are represented by virtual functions in this class.
*/
class ObjectClass : public AbstractClass
{
	public:
		/*
		**	The object can be in one of two states -- placed down on the map, or not. If the
		**	object is placed down on the map, then this flag will be true.
		*/
		unsigned IsDown:1;

		/*
		**	This is a support flag that is only used while building a list of objects to
		**	be damaged by a proximity affect (explosion). When this flag is set, this object
		**	will not be added to the list of units to damage. When damage is applied to the
		**	object, this flag is cleared again. This process ensures that an object is never
		**	subject to "double jeopardy".
		*/
		unsigned IsToDamage:1;

		/*
		**	Is this object flagged to be displayed during the next rendering process?  This
		**	flag could be set by many different circumstances. It is automatically cleared
		**	when the object is rerendered.
		*/
		unsigned IsToDisplay:1;

		/*
		**	An object in the game may be valid yet held in a state of "limbo". Units are in such
		**	a state if they are being transported or are otherwise "inside" another unit. They can
		**	also be in limbo if they have been created but are being held until the proper time
		**	for delivery.
		*/
		unsigned IsInLimbo:1;

		/*
		**	When an object is "selected" it is given a floating bar graph or other graphic imagery
		**	to display this fact. When the player performs I/O, the actions may have a direct
		**	bearing on the actions of the currently selected object. For quick checking purposes,
		**	if this object is the one that is "selected", this flag will be true.
		*/
		unsigned IsSelected:1;

		//Added a mask instead of bool for selecting players. This is because we must now support multiplayer.
		// - 6/26/2019
		// Needs more than 16 bits since RA has more than 16 factions. ST - 8/14/2019 12:15PM
		unsigned int IsSelectedMask;

		/*
		**	If an animation is attached to this object, then this flag will be true.
		*/
		unsigned IsAnimAttached:1;

		/*
		**	If this object should process falling logic, then this flag will be true. Such
		**	objects might be ballistic projectiles, grenades, or parachuters.
		*/
		unsigned IsFalling:1;
		int Riser;

		/*
		**	Several objects could exist in the same cell list. This is a pointer to the
		**	next object in the cell list. The objects in this list are not in any
		**	significant order.
		*/
		SmartPtr<ObjectClass> Next;

		/*
		** Every object can be assigned a trigger; the same trigger can be assigned
		** to multiple objects.
		*/
		CCPtr<TriggerClass> Trigger;

		/*
		**	This is the current strength of this object.
		*/
		short Strength;

		/*
		** Some additional padding in case we need to add data to the class and maintain backwards compatibility for save/load
		*/
		unsigned char SaveLoadPadding[16];

		/*-----------------------------------------------------------------------------------
		**	Constructor & destructors.
		*/
		ObjectClass(RTTIType rtti, int id);
		ObjectClass(NoInitClass const & x) : AbstractClass(x), Next(x), Trigger(x) {};
		virtual ~ObjectClass(void) {Next = 0;};
		int operator < (ObjectClass const & object) const {return Sort_Y() < object.Sort_Y();};
		int operator > (ObjectClass const & object) const {return Sort_Y() > object.Sort_Y();};

		/*
		**	Object selection control.
		*/
		static void Init(void);

		/*
		**	Query functions.
		*/
		virtual bool Is_Players_Army(void) const {return(false);}
		virtual void const * Get_Image_Data(void) const;
		virtual ActionType What_Action(ObjectClass const *) const;
		virtual ActionType What_Action(CELL) const;
		virtual LayerType In_Which_Layer(void) const;
		bool Is_Infantry(void) const {return(RTTI == RTTI_INFANTRY);};
		bool Is_Foot(void) const {return(RTTI == RTTI_INFANTRY || RTTI == RTTI_UNIT || RTTI == RTTI_VESSEL || RTTI == RTTI_AIRCRAFT);};
		bool Is_Techno(void) const {return(RTTI == RTTI_BUILDING || RTTI == RTTI_UNIT || RTTI == RTTI_INFANTRY || RTTI == RTTI_VESSEL || RTTI == RTTI_AIRCRAFT);};
		virtual int Get_Ownable(void) const;
		virtual ObjectTypeClass const & Class_Of(void) const = 0;
		virtual bool Class_Is_Valid(void) const { return false; }
		virtual char const * Name(void) const;
		virtual int Full_Name(void) const;
		virtual bool Can_Repair(void) const;
		virtual bool Can_Demolish(void) const;
		virtual bool Can_Demolish_Unit(void) const;
		virtual bool Can_Capture(void) const;
		virtual bool Can_Player_Fire(void) const;
		virtual bool Can_Player_Move(void) const;

		/*
		**	Coordinate inquiry functions. These are used for both display and
		**	combat purposes.
		*/
		virtual COORDINATE Docking_Coord(void) const;
		virtual COORDINATE Target_Coord(void) const;
		virtual COORDINATE Center_Coord(void) const;
		virtual COORDINATE Render_Coord(void) const;
		virtual COORDINATE Sort_Y(void) const;
		virtual FireDataType Fire_Data(int which) const;
		virtual COORDINATE Fire_Coord(int which) const;
		virtual COORDINATE Exit_Coord(void) const;

		/*
		**	Object entry and exit from the game system.
		*/
		virtual bool Limbo(void);
		virtual bool Unlimbo(COORDINATE , DirType facing = DIR_N);
		virtual void Detach(TARGET target, bool all=true);
		virtual void Detach_All(bool all=true);
		virtual void Record_The_Kill(TechnoClass * );
		virtual bool Paradrop(COORDINATE coord);
		bool Attach_Trigger(TriggerClass * trigger);

		/*
		**	Display and rendering support functionality. Supports imagery and how
		**	object interacts with the map and thus indirectly controls rendering.
		*/
		virtual void Do_Shimmer(void);
		virtual int Exit_Object(TechnoClass *);
		virtual bool Render(bool forced) const;
		virtual short const * Occupy_List(bool placement=false) const;
		virtual short const * Overlap_List(bool redraw=false) const;
		virtual fixed Health_Ratio(void) const;
		virtual void Draw_It(int x, int y, WindowNumberType ) const = 0;
		virtual void Hidden(void);
		virtual void Look(bool incremental=false);
		virtual bool Mark(MarkType=MARK_CHANGE);

	private:
		virtual void Mark_For_Redraw(void);

	public:

		/*
		**	User I/O.
		*/
		virtual void Active_Click_With(ActionType , ObjectClass *);
		virtual void Active_Click_With(ActionType , CELL );
		virtual void Clicked_As_Target(HousesType house, int = 7); // 2019/09/20 JAS - Added record of who clicked on the object
		virtual bool Select(bool allow_mixed = false);
		virtual void Unselect(void);

		//These selection functions were added to handle the fact that we now need to support 
		//client-server multiplayer. - JAS 6/26/2019
		virtual void Unselect_All_Players(void);
		virtual void Unselect_All_Players_Except_Owner(void);
		virtual bool Is_Selected_By_Player(HouseClass *player = NULL) const;
		virtual void Set_Selected_By_Player(HouseClass *player = NULL);
		virtual void Set_Unselected_By_Player(HouseClass *player = NULL);

		/*
		**	Combat related.
		*/
		virtual bool In_Range(COORDINATE , int=0) const;
		virtual int Weapon_Range(int =0) const;
		virtual ResultType Take_Damage(int & damage, int distance, WarheadType warhead, TechnoClass * source=0, bool forced=false);
		virtual void Scatter(COORDINATE , bool forced=false, bool nokidding=false);
		virtual bool Catch_Fire(void);
		virtual void Fire_Out(void);
		virtual int Value(void) const;
		virtual MissionType Get_Mission(void) const;

		/*
		**	AI.
		*/
		virtual void Per_Cell_Process(PCPType) {}
		virtual BuildingClass * Who_Can_Build_Me(bool intheory, bool legal) const;
		virtual RadioMessageType Receive_Message(RadioClass * from, RadioMessageType message, long & param);
		virtual bool Revealed(HouseClass * house);
		virtual void Repair(int );
		virtual void Sell_Back(int );
		virtual void AI(void);

		/*
		**	File I/O.
		*/
		virtual void Code_Pointers(void);
		virtual void Decode_Pointers(void);

		/*
		**	Scenario and debug support.
		*/
		#ifdef CHEAT_KEYS
		virtual void Debug_Dump(MonoClass *mono) const;
		#endif
		virtual void Move(FacingType);

		enum {FLIGHT_LEVEL=256};

};

/*
**	Project Aeloria — TEMPORARY BAND-AID for early-lifecycle object corruption (ScenarioInit window)
**
**	After the 2026 packing/ODR fix, some objects (Terrain Raw=15, Units, etc.) are still reaching
**	the first Map.Render() with a plausible declared Class.Raw() but garbage at the runtime offset
**	the drawing code actually dereferences (+8 from most-derived this, observed as 0x00210000,
**	0x80738000, other 0x80xxxxxx / 0x81xxxxxx patterns).
**
**	This cheap predicate is the evidence-based filter extracted from live debug data.
**	It is intentionally placed in OBJECT.H so it can be shared by:
**	  - All 8 Class_Is_Valid() overrides
**	  - The two CC_Draw_Shape(ObjectClass*) entry points (last-line defense)
**	  - Future creation-time or Cell pruning checks
**
**	High-byte rejection (0x00/0x80/0x81) + basic alignment covers every crash pattern seen
**	in the Aeloria_Debug.log and debugger watches as of 2026-05-17.
**
**	THIS IS A TEMPORARY MITIGATION, NOT A ROOT-CAUSE FIX.
**	The real problem is that the drawing pipeline (and C#/interop blitter path) still assumes
**	a legacy memory layout for the Class pointer inside instance objects during the narrow
**	"objects placed but not fully initialized" window after CNC_Start_Instance_Variation.
**
**	When the root cause is finally addressed (consistent layout, epoch flag, or moving the
**	effective pointer read), delete this helper and all call sites. Leave the band-aid comments
**	in the git history as documentation of the post-packing symptom.
**
**	See plan.md "Expert Panel Review" section (May 2026) for the full rationale from the
**	4-expert panel (Low-Level/ABI, C++ Legacy, Game Systems, QE).
*/

// Pragmatic Option A (May 2026): one-frame early-load grace flag.
// When true (only during the very first Map.Render() after ScenarioInit drops to 0),
// Is_Plausible_Class_Pointer returns true without checking the +8 offset.
// This lets objects that pass the declared Class checks render on the first frame
// (fixing the "no units on load" symptom) while the full +8 guard remains active
// for every subsequent frame.
extern bool g_EarlyLoadGraceActive;

// Pragmatic Option A continuation — Phase B (May 2026): much longer player House exemption.
// 18000 frames (~5 minutes of logic time) for human-owned objects during early load.
// Combined with the "use safe default size instead of hard skip" relaxation for the human
// house only, this restores normal visual speed (no more 5% throttling from suppressed draws)
// while still protecting against blitter AVs on corrupted early-created player units.
// Non-player objects (Terrain, etc.) remain under the strict shape sanity guard.
extern HousesType g_HumanPlayerHouse;
extern int g_PlayerExemptionFrames;

// Phase B instrumentation (May 2026) — human player draw statistics during the long exemption window.
extern int g_HumanPlayerDrawAttempts;
extern int g_HumanPlayerFallbackUses;

// New counters for richer diagnostics (Items 3 & 7)
extern int g_AeloriaPlayerGuardHits;
extern int g_AeloriaNonPlayerGuardHits;
extern int g_AeloriaPromotedThenDemoted;

extern DWORD g_FirstRenderTick;   // Set at the very first Map.Render() after ScenarioInit

// Project Aeloria: per-object, per-message-type "log once" tracker for verbose draw diagnostics
// during the long human-player exemption window. Keyed by (this pointer) -> bitmask of seen tags.
// This replaces the previous Frame%12 sampling so that after seeing each distinct message
// ("guard passed", "about to call Techno_Draw_Object", "PLAYER EXEMPTION bypassing", etc.)
// once per object we stay silent for that (obj, msg) pair. Keeps total log 50-500 lines
// after minutes of play while preserving every counter increment and SEVERE/rate-limited events.
extern std::map<uintptr_t, uint32_t> g_AeloriaObjectLogMask;

// Per-object stabilization tracking (first priority hardening).
// Tracks individual objects' success at drawing with real shape data.
// Goal: Allow most objects to "graduate" out of the heavy global exemption path
// instead of forcing every player object through the full 18000-frame timer.
struct AeloriaObjectStability {
    uint16_t successfulRealDraws = 0;   // number of draws with real (w>0 && h>0) shape data
    uint16_t fallbackUses = 0;          // number of times we had to use safe placeholder (bad shape during exemption)
    uint8_t  stabilityLevel = 0;        // 0=unstable, 1=stabilizing, 2=stable (graduated)
    uint32_t lastGoodFrame = 0;
    uint8_t  rtti = 0;                  // cached What_Am_I() for richer summaries (Item 3)
    bool     earlySafeClientRegistered = false; // Phase 2 (decoupled safe registration): true once this object has been given a usable CNCObjectStruct via the client path using only reliable instance data (What_Am_I/Owner/RTTI fallbacks + coords). Explicit state lets us graduate out of exemption and stop relying on pointer heuristics for the player's view/selection.
    bool     clientListInserted = false;        // Set true when this hasCreation starting unit has been directly written into a real client ObjectList slot (bypasses timing issues with draw intercept early returns).
    bool     sustainRetired = false;            // True once normal layer export reliably includes this unit (or it has a valid MAIN draw cache); stops per-export sustain bulk work.
    uint8_t  sustainNormalHits = 0;             // Consecutive exports where the normal layer walk already contained this object.
    // Last good virtual-draw params from DLL_Draw_Intercept — sustain bulk re-insert reads these so
    // the client gets real ShapeIndex/AssetName/dims instead of generic E1+shape 0 (invisible sprites).
    bool     hasCachedDraw = false;       // virtual/client intercept params (sustain bulk display)
    bool     hasCachedMainDraw = false;   // confirmed legacy MAIN tactical blitter draw (graduation gate)
    int      cachedShapeNumber = 0;
    int      cachedDrawFlags = 0;
    unsigned char cachedRotation = 0;
    long     cachedScale = 0x100;
    int      cachedDrawX = 0;
    int      cachedDrawY = 0;
    int      cachedWidth = 0;
    int      cachedHeight = 0;
    char     cachedAssetName[16] = {};
    // Type enum captured at Unlimbo when Class resolves (UnitType / InfantryType value).
    int16_t  cachedTypeEnum = -1;
    // Phase 5i: war-factory produced unit — first Draw_It diagnostics (bit0=VIRTUAL, bit1=MAIN).
    bool     producedUnitUnlimboSeeded = false;
    bool     producedUnitBadPlus8 = false;
    uint8_t  producedUnitFirstDrawMask = 0;
};
extern std::map<uintptr_t, AeloriaObjectStability> g_AeloriaObjectStability;

// Per-object creation frame tracking (Item 1 in Logging QoL plan).
// Records the Frame at which a player-relevant object was Unlimbo'd.
// Used to answer "was this unit bad from birth or did it go bad later?"
// Cleared together with g_AeloriaObjectStability.
extern std::map<uintptr_t, uint32_t> g_AeloriaObjectCreationFrame;

// Global master kill-switch for the remaining verbose per-draw diagnostics ("got Object",
// "guard passed", "about to call ...", GRACE FRAME, PLAYER EXEMPTION bypass, etc.).
// When false, Aeloria_ShouldLogVerbose returns false immediately and all direct
// once-per verbose log sites are also suppressed. This eliminates the first-60s I/O
// tax and the multi-hundred-thousand-line log spam during playtests while leaving
// SEVERE corruption reports, counters, creation logs, and the final window-expiration
// stats line fully intact.
// Default true so diagnosis builds stay chatty; set to false for clean play.
extern bool g_AeloriaEnableVerboseDrawLogs;
// infantry-scale Phase 2: skip stability/creation maps for healthy barracks-produced infantry.
extern bool g_AeloriaZeroMapProducedInfantry;
// infantry-scale Phase 3b: stateless client intercept for untracked techno with healthy Class.Raw().
extern bool g_AeloriaStatelessUntrackedTechno;

// Distinct message tags for the once-per-object suppression (bit positions).
// Only the high-volume per-frame draw diagnostics get unique tags here.
enum AeloriaLogTag {
	AEL_LOG_NONE = 0,
	AEL_LOG_GUARD_PASSED = 1,
	AEL_LOG_GET_IMAGE_DATA = 2,
	AEL_LOG_SHAPE_NUMBER = 3,
	AEL_LOG_TECHNO_DRAW_OBJECT = 4,
	AEL_LOG_DRAWING_UNIT_RADAR = 5,
	AEL_LOG_GRACE_TECHNO_DRAW = 6,
	AEL_LOG_CC_DRAW_GOT_OBJECT = 7,
	AEL_LOG_GRACE_FINAL = 8,
	AEL_LOG_GRACE_SKIP = 9,
	AEL_LOG_PLAYER_EXEMPT_BYPASS = 10,
	AEL_LOG_GRACE_RADAR_DISH = 11,
	AEL_LOG_TERRAIN_ABOUT_TO_CC = 12,
	AEL_LOG_PLAYER_PHASE_B_SEVERE = 13,
	AEL_LOG_PLAYER_CREATION = 14,     // Item 1
	AEL_LOG_FIRST_REAL_DRAW = 15,     // Item 1
	AEL_LOG_FIRST_FALLBACK  = 16,     // Item 1
	AEL_LOG_INFANTRY_DEATH  = 17,     // Phase 5j: once per death-anim MAIN draw
	AEL_LOG_ACCESSORY_DRAW  = 18,     // Phase 5z-l: once per accessory sub-draw (WEAP2, turrets)
	AEL_LOG_PLACEHOLDER_ENTER = 19,   // Phase 5z-n9 PR4: once per DrawSafePlaceholder entry
	AEL_LOG_PLACEHOLDER_MARKER = 20,  // Phase 5z-n9 PR4: once per yellow visibility marker log
	// Room for more
	AEL_LOG_MAX = 31
};

// Real-time based gate for heavy diagnostic spam on non-player objects.
// After the first render + ~8 real seconds, we permanently quiet the repetitive
// "guard passed / about to call / GRACE FRAME FINAL / SKIP" logs for everything
// that is not a human player object. This is the only way to stop the flood
// when the game is running at 5% speed (Frame advances too slowly).
inline bool ShouldLogHeavyNonPlayerDiagnostics()
{
    if (g_FirstRenderTick == 0) return true;
    return (GetTickCount() - g_FirstRenderTick) < 8000;   // 8 real seconds
}
const int PLAYER_EXEMPTION_FRAME_COUNT = 600;   // ~40s logic frames for opening; per-object graduation clears most units earlier
const int AELORIA_SUSTAIN_MAX_LAYER_EXPORTS = 12; // Cap sustain re-feeds after skirmish-start window

inline bool Is_Plausible_Class_Pointer(uintptr_t ptr)
{
	// Pragmatic Option A: during the single first-render grace frame we trust the
	// declared Class checks (already performed by the caller) and skip the +8 probe.
	// This restores visibility of starting forces on load without weakening the
	// guard for normal gameplay.
	if (g_EarlyLoadGraceActive)
		return true;

	if (ptr == 0) return false;

	// Reject tiny garbage values (Class.Raw=1 etc.) that are non-null but not real pointers.
	if (ptr < 0x10000) return false;

	// Win32 user-mode addresses live below 0x80000000. Kernel/sentinel garbage at +8
	// (0x80xxxxxx MCV @ frame 73, 0x88xxxxxx, 0xb010b000 / 0xb810b800 V2 @ frame 135) must not pass.
	if (ptr >= 0x80000000u) return false;

	// Reject low sentinel garbage (0x00210000 family, tiny non-null values in +8 peek).
	unsigned char high = (unsigned char)((ptr >> 24) & 0xFF);
	if (high == 0x00) return false;

	// Basic alignment sanity (TypeClass* should be at least 4-byte aligned on Win32)
	if ((ptr & 0x3) != 0) return false;

	return true;
}

/*
**	Project Aeloria player exemption helper (tiny, always-approve owner path + timeout).
**	Returns true only for human player's Techno (and other) objects while the short
**	countdown or initial grace is active. Owner() is safe here (visible on ObjectClass).
**	Used in CONQUER.CPP skip paths for loud bypass logging + relaxed checks.
**	No log inside helper (would be too noisy); caller logs on actual exemption events.
*/
// True only when a MAIN-window draw cached non-zero width AND height (not bulk defaults).
inline bool Aeloria_HasValidMainDrawCache(const ObjectClass* obj)
{
	if (obj == nullptr) return false;
	uintptr_t key = reinterpret_cast<uintptr_t>(obj);
	auto it = g_AeloriaObjectStability.find(key);
	if (it == g_AeloriaObjectStability.end()) return false;
	const AeloriaObjectStability& s = it->second;
	// Virtual-window intercept can cache shape>0 while +8 is still corrupt on MAIN (1836 session).
	// Only a confirmed MAIN tactical blitter draw clears the starting-unit MAIN guard.
	return s.hasCachedMainDraw && s.cachedWidth > 0 && s.cachedHeight > 0 && s.cachedShapeNumber > 0;
}

// SHAPE_CENTER (0x0020) | SHAPE_WIN_REL (0x0010) — matches Techno_Draw_Object / CC_Draw_Shape.
// The Remaster client treats PositionX/Y as sprite top-left unless SHAPE_CENTER is in DrawFlags,
// but Coord_To_Pixel and Draw_It always supply center pixel coordinates.
#define AELORIA_CLIENT_DRAW_FLAGS_CENTER 0x0030

void Aeloria_NotifyMainDrawCache(const ObjectClass* obj, int shape_number, int width, int height, int draw_x, int draw_y);
bool Aeloria_TryNotifyProducedUnitMainDrawCache(const ObjectClass* obj, int shape_number, int width, int height,
                                                int draw_x, int draw_y, bool runtime_bad_plus_8);
void Aeloria_GraduateTrackedObject(const ObjectClass* obj);
int Aeloria_PeekTechnoTypeEnum(TechnoClass const* techno);

inline bool Aeloria_IsTrackedStartingUnit(const ObjectClass* obj)
{
	if (obj == nullptr) return false;
	uintptr_t key = reinterpret_cast<uintptr_t>(obj);
	return g_AeloriaObjectCreationFrame.find(key) != g_AeloriaObjectCreationFrame.end();
}

// Refinery-spawned harvester reusing a starting-unit pool slot (e.g. former MCV).
inline bool Aeloria_IsRepurposedHarvester(const ObjectClass* obj)
{
	if (obj == nullptr || obj->What_Am_I() != RTTI_UNIT) return false;
	uintptr_t key = reinterpret_cast<uintptr_t>(obj);
	auto it = g_AeloriaObjectStability.find(key);
	if (it == g_AeloriaObjectStability.end()
	    || it->second.cachedTypeEnum != (int16_t)UNIT_HARVESTER
	    || !it->second.earlySafeClientRegistered) {
		return false;
	}
	// Relocate clears hasCachedMainDraw, but stale MAIN cache from the prior pool
	// occupant (e.g. MCV) can survive if tracking was not reset on death — always
	// guard repurposed harvesters regardless of Aeloria_HasValidMainDrawCache.
	return true;
}

inline bool Aeloria_IsProducedTechnoUnlimboSeeded(const ObjectClass* obj)
{
	if (obj == nullptr) return false;
	uintptr_t key = reinterpret_cast<uintptr_t>(obj);
	auto it = g_AeloriaObjectStability.find(key);
	return it != g_AeloriaObjectStability.end() && it->second.producedUnitUnlimboSeeded;
}

// Back-compat alias — WF vehicles and mid-game aircraft (spy plane, helipad) share this flag.
inline bool Aeloria_IsProducedUnitUnlimboSeeded(const ObjectClass* obj)
{
	return Aeloria_IsProducedTechnoUnlimboSeeded(obj);
}

// WF / reinforcement aircraft with corrupt +8 at Unlimbo — never use legacy blitter for the match.
inline bool Aeloria_IsProducedBadPlus8Unit(const ObjectClass* obj)
{
	if (obj == nullptr) return false;
	RTTIType rtti = obj->What_Am_I();
	if (rtti != RTTI_UNIT && rtti != RTTI_AIRCRAFT && rtti != RTTI_VESSEL) return false;
	uintptr_t key = reinterpret_cast<uintptr_t>(obj);
	auto it = g_AeloriaObjectStability.find(key);
	return it != g_AeloriaObjectStability.end()
	    && it->second.producedUnitUnlimboSeeded
	    && it->second.producedUnitBadPlus8;
}

// WF-produced harvesters with bad +8: eternal safe draw like refinery-spawned pool-reuse harvesters.
inline bool Aeloria_IsProducedBadPlus8Harvester(const ObjectClass* obj)
{
	if (!Aeloria_IsProducedBadPlus8Unit(obj)) return false;
	uintptr_t key = reinterpret_cast<uintptr_t>(obj);
	auto it = g_AeloriaObjectStability.find(key);
	return it != g_AeloriaObjectStability.end()
	    && it->second.cachedTypeEnum == (int16_t)UNIT_HARVESTER;
}

// WF/shipyard-produced vehicles and vessels: eternal safe draw (5z-n8/n10).
// Aircraft (5z-m10): safe intercept until sustainRetired; bad+8 eternal; legacy MAIN only after handoff.
inline bool Aeloria_IsEternalSafeProducedUnit(const ObjectClass* obj)
{
	if (obj == nullptr) return false;
	RTTIType rtti = obj->What_Am_I();
	if (rtti != RTTI_UNIT && rtti != RTTI_AIRCRAFT && rtti != RTTI_VESSEL) return false;
	if (Aeloria_IsRepurposedHarvester(obj)) return true;
	uintptr_t key = reinterpret_cast<uintptr_t>(obj);
	auto it = g_AeloriaObjectStability.find(key);
	if (it == g_AeloriaObjectStability.end() || !it->second.producedUnitUnlimboSeeded) {
		return false;
	}
	if (rtti == RTTI_AIRCRAFT) {
		return it->second.producedUnitBadPlus8;
	}
	return true;
}

// Human MCV/sidebar-placed buildings: type cached at Unlimbo, corrupt +8 persists for the whole match.
inline bool Aeloria_IsHumanDeployedBuilding(const ObjectClass* obj)
{
	if (obj == nullptr || g_HumanPlayerHouse == HOUSE_NONE) return false;
	if (obj->Owner() != g_HumanPlayerHouse || obj->What_Am_I() != RTTI_BUILDING) return false;
	uintptr_t key = reinterpret_cast<uintptr_t>(obj);
	auto it = g_AeloriaObjectStability.find(key);
	return it != g_AeloriaObjectStability.end() && it->second.cachedTypeEnum >= 0;
}

// Human-deployed building may use legacy MAIN blitter only after MAIN draw cache is valid and
// stab.cachedTypeEnum still matches live building type (5z-n9 PR2).
inline bool Aeloria_HasTrustedHumanBuildingMainCache(const ObjectClass* obj)
{
	if (!Aeloria_IsHumanDeployedBuilding(obj)) return false;
	if (!Aeloria_HasValidMainDrawCache(obj)) return false;
	uintptr_t key = reinterpret_cast<uintptr_t>(obj);
	auto it = g_AeloriaObjectStability.find(key);
	if (it == g_AeloriaObjectStability.end()) return false;
	int cachedType = (int)it->second.cachedTypeEnum;
	if (cachedType < 0) return false;
	int currentType = Aeloria_PeekTechnoTypeEnum(reinterpret_cast<TechnoClass const*>(obj));
	if (currentType < 0) return false;
	return cachedType == currentType;
}

// Starting units seeded at Unlimbo must not hit the legacy MAIN blitter until a confirmed-safe
// MAIN-window draw (shape>0) has been cached. Virtual cache alone is not enough.
// Also covers earlySafeClientRegistered units that were prematurely graduated off the creation
// map when +8 briefly looked plausible (0x684f6800-style decay) — the 1826 move-order crash.
inline bool Aeloria_IsEarlyStartingUnitMainGuarded(const ObjectClass* obj)
{
	if (obj == nullptr) return false;
	if (Aeloria_IsHumanDeployedBuilding(obj)) {
		if (Aeloria_HasTrustedHumanBuildingMainCache(obj)) return false;
		return true;
	}
	if (Aeloria_HasValidMainDrawCache(obj)) return false;
	// Eternal-safe produced / repurposed units use the WF safe-client path, not starting-unit placeholder guard.
	if (Aeloria_IsRepurposedHarvester(obj) || Aeloria_IsEternalSafeProducedUnit(obj)) return false;
	uintptr_t key = reinterpret_cast<uintptr_t>(obj);
	if (g_AeloriaObjectCreationFrame.find(key) != g_AeloriaObjectCreationFrame.end()) {
		// 5z-m7: mid-game airfield/helipad aircraft with plausible +8 use normal draw + bulk, not intercept guard.
		auto it = g_AeloriaObjectStability.find(key);
		if (it != g_AeloriaObjectStability.end()
		    && it->second.producedUnitUnlimboSeeded
		    && obj->What_Am_I() == RTTI_AIRCRAFT
		    && !it->second.producedUnitBadPlus8) {
			return false;
		}
		return true;
	}
	auto it = g_AeloriaObjectStability.find(key);
	return it != g_AeloriaObjectStability.end() && it->second.earlySafeClientRegistered;
}

// MAIN/tactical blitter must not run for starting-unit or produced-unit guards.
inline bool Aeloria_NeedsSafeMainDrawGuard(const ObjectClass* obj)
{
	return Aeloria_IsEarlyStartingUnitMainGuarded(obj) || Aeloria_IsProducedBadPlus8Unit(obj);
}

inline bool Is_Player_Exempt(const ObjectClass* obj)
{
	if (obj == nullptr || g_HumanPlayerHouse == HOUSE_NONE)
		return false;

	// Per-object stabilization check (new hardening)
	uintptr_t key = reinterpret_cast<uintptr_t>(obj);
	auto it = g_AeloriaObjectStability.find(key);
	if (it != g_AeloriaObjectStability.end() && it->second.stabilityLevel >= 2) {
		// Bulk-registered starting units can be promoted to level 2 before any real shape
		// draw succeeds; keep exemption until we have cached draw params from a real draw.
		if (!Aeloria_HasValidMainDrawCache(obj)) {
			if (Aeloria_IsHumanDeployedBuilding(obj) || g_PlayerExemptionFrames > 0 || Frame < 600) {
				return true;
			}
		} else {
			return false; // Object has stabilized with real draw data
		}
	}
	// Phase 2: graduate only after safe client registration AND a real cached draw.
	if (it != g_AeloriaObjectStability.end() && it->second.earlySafeClientRegistered
	    && Aeloria_HasValidMainDrawCache(obj)) {
		return false;
	}

	// Step 2/3: all bulk-registered starting units (human + AI) stay exempt until real MAIN draw cache exists.
	if (it != g_AeloriaObjectStability.end() && it->second.earlySafeClientRegistered
	    && !Aeloria_HasValidMainDrawCache(obj)) {
		if (g_PlayerExemptionFrames > 0 || Frame < 600) {
			return true;
		}
	}

	if (Aeloria_IsHumanDeployedBuilding(obj) && !Aeloria_HasValidMainDrawCache(obj)) {
		return true;
	}

	if (g_PlayerExemptionFrames <= 0 && !g_EarlyLoadGraceActive)
		return false;

	HousesType owner = obj->Owner();
	// Human house gets the full long exemption + safe placeholder treatment.
	// (Allied house extension temporarily removed to keep compile clean; can be added
	// with proper includes later if ally visibility remains an issue.)
	return (owner == g_HumanPlayerHouse);
}

/*
**	Project Aeloria logging throttle (May 2026, final log-once policy).
**	Per the approved "log once per object per message" plan: each distinct verbose draw diagnostic
**	(guard passed, about-to-call Techno_Draw, PLAYER EXEMPTION bypass, GRACE FRAME*, DRAWING UNIT radar, etc.)
**	is emitted at most once per object pointer during the exemption window. Achieves the target of
**	50-500 line logs after minutes of play instead of hundreds of thousands.
**
**	Player objects: ShouldLogVerbose returns true; the per-tag once filter in ShouldLogOncePerObject
**	does the actual dedup (using g_AeloriaObjectLogMask bitmask).
**	Non-player: real-time ShouldLogHeavyNonPlayerDiagnostics() + opening burst.
**	Counters, SEVERE (rate-limited), and window-expiration stats are never suppressed.
*/
inline bool Aeloria_ShouldLogVerbose(const ObjectClass* obj)
{
	if (obj == nullptr) return false;
	if (!g_AeloriaEnableVerboseDrawLogs) return false;  // Master toggle off -> silence all per-draw spam (first-60s I/O + hundreds of k lines)
	if (Is_Player_Exempt(obj)) return true;   // Full verbosity decision now delegated to Aeloria_ShouldLogOncePerObject (log each msg type once per object)
	if (Frame < 600) return true;             // Full detail for the first 10 seconds (non-player only)
	return false;
}

// Returns true only the *first* time this (object pointer, message tag) pair is seen while the
// player exemption window is active. Subsequent identical messages for the same `this` are suppressed.
// Non-player objects fall back to the normal verbose/heavy gate logic (already rate-limited by real time).
bool Aeloria_ShouldLogOncePerObject(const void* obj, int tag);

// Safe TechnoTypeClass lookup for early custom-map units whose CCPtr Class can be null
// while the object is still active/selectable (avoids AV in per-frame AI / move logic).
class TechnoTypeClass;
TechnoTypeClass const* Aeloria_Safe_Techno_Type(TechnoClass const* techno);
void Aeloria_Repair_Early_Class_Pointer(TechnoClass* techno);
void Aeloria_Seed_Building_Creation(BuildingClass* building);
void Aeloria_RefreshBuildingStabilityOnSeed(BuildingClass* building, bool humanDeployed);
void Aeloria_Seed_Human_Building_Stability(BuildingClass* building);

// 5z-l: Accessory layer (WEAP2 roof, tank/vessel turrets) on guarded safe-draw paths.
// Does not touch main-draw cache or graduation; uses explicit shape_file_name for client export.
void Aeloria_DrawAccessoryIntercept(const ObjectClass* obj, int shapenum, int x, int y,
                                    int drawW, int drawH, const char* shape_file_name,
                                    char override_owner = HOUSE_NONE);

struct CNCObjectStruct;
void Aeloria_PopulateTechnoHudFields(CNCObjectStruct& slot, const TechnoClass* tc);
void Aeloria_SyncVirtualSelectionHud(const TechnoClass* tc);
void Aeloria_RecordProducedAircraftVirtualEmit(const ObjectClass* obj);

/*
**	Project Aeloria (Draw Guard helper):
**	Centralized minimal helper to reduce duplication of the `if (!Class.Is_Valid()) return;`
**	early-outs across Draw_It() implementations (and potentially other drawing paths).
**	Implemented as a templated free function in the ObjectClass header (most general location)
**	because the 'Class' CCPtr member is declared only in the concrete derived classes
**	(e.g. UnitClass, TerrainClass). The template is instantiated with the concrete type at
**	the call site (inside each Draw_It), providing direct access to its Class and inherited
**	Get_Image_Data(). Combines both the Class and image validity checks per the suggested
**	signature. Uses Is_ prefix per project STYLE.H convention for boolean queries.
**	Placed here (not TechnoClass) for full coverage including TerrainClass.
*/
template<class T>
inline bool Is_Drawable(const T * obj)
{
	if (!obj) return false;

	// Delegate all validity logic to the object itself.
	// Each class that has a Class member implements Class_Is_Valid()
	// with the appropriate checks (ID > 0, pointer sanity, TypeClass fields, etc.).
	if (!obj->Class_Is_Valid()) return false;

	// Safe to call Get_Image_Data() now
	if (obj->Get_Image_Data() == NULL) return false;

	return true;
}

#endif