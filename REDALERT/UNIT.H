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

/* $Header: /CounterStrike/UNIT.H 1     3/03/97 10:26a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer                                            *
 *                                                                                             *
 *                    File Name : UNIT.H                                                       *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic                                                *
 *                                                                                             *
 *                   Start Date : April 14, 1994                                               *
 *                                                                                             *
 *                  Last Update : April 14, 1994   [JLB]                                       *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef UNIT_H
#define UNIT_H

#include "drive.h"
#include "radio.h"
#include "cargo.h"
#include "mission.h"
#include	"target.h"


/****************************************************************************
**	For each instance of a unit (vehicle) in the game, there is one of
**	these structures. This structure holds information that is specific
**	and dynamic for a particular unit.
*/
class UnitClass :	public DriveClass
{
	public:

		/*
		**	This points to the static control data that gives 'this' unit its characteristics.
		*/
		CCPtr<UnitTypeClass> Class;

		/*
		**	This records the house flag that this object is currently carrying.
		*/
		HousesType Flagged;

		/*
		** This flag is used for when the harvester dumps ore, to track its
		** special animation.
		*/
		unsigned IsDumping:1;

		/*
		** This is a count of the # of loads of the various minerals that the
		** unit has harvested.
		*/
		unsigned Gold:5;
		unsigned Gems:5;

		/*
		** This flag tells a unit that, if after reaching its destination, it
		** should scatter away.  It's meant to help a LST unload its units by
		** having its previous passengers get out of the way.
		*/
		unsigned IsToScatter:1;

		/*
		**	This records the number of "loads" of Tiberium the unit is carrying. Only
		**	harvesters use this field.
		*/
		int Tiberium;

		/*
		** This is the area where a mobile gap generator stores the previously-held
		** shroud values for the cells surrounding itself.
		*/
		unsigned long ShroudBits;

		/*
		** This is the center coordinate for the mobile gap generator, as to
		** what cells should be revealed (according to ShroudBits)
		*/
		CELL ShroudCenter;

		/*
		**	This is the timer that controls the reload rate. The MSAM rocket
		**	launcher is the primary user of this.
		*/
		CDTimerClass<FrameTimerClass> Reload;

		/*
		**	This is the facing of the turret. It can be, and usually is,
		**	rotated independently of the body it is attached to.
		*/
		FacingClass SecondaryFacing;

		/*
		**	Project Aeloria / Rampastring QoL:
		**	The coordinate of the last docking bay (refinery) this harvester used.
		**	Harvesters will prefer returning to this refinery (or a nearby one) when
		**	looking for a place to unload. This helps reduce long travel times between
		**	harvest patches and distant refineries.
		*/
		COORDINATE LastDockingBayCoord;

		// Note: ArchiveTarget is a general member used by multiple mission types.
		// For harvesters, it is repurposed as "last successful harvest location" memory.
		// Access should go through the helper methods below for clarity.

		/*
		**	Project Aeloria / Rampastring QoL:
		**	Flag set when a harvester unloads at a refinery that has a rally point.
		**	Causes the harvester to immediately begin searching for a new Tiberium field
		**	(after it reaches the rally point) instead of idling. This creates a smooth
		**	"unload → move to rally → harvest again" cycle.
		*/
		bool InitHarvest;

		/*
		** Some additional padding in case we need to add data to the class and maintain backwards compatibility for save/load
		*/
		unsigned char SaveLoadPadding[27];

		/*---------------------------------------------------------------------
		**	Constructors, Destructors, and overloaded operators.
		*/
		static void * operator new(size_t size);
		static void * operator new(size_t , void * ptr) {return(ptr);};
		static void operator delete(void *ptr);
		UnitClass(UnitType classid, HousesType house);
		UnitClass(NoInitClass const & x) : DriveClass(x), Class(x), Reload(x), SecondaryFacing(x) {};
		operator UnitType(void) const {
			UnitTypeClass const* t = Class;
			return t ? t->Type : UNIT_NONE;
		};
		virtual ~UnitClass(void);

		/*---------------------------------------------------------------------
		**	Member function prototypes.
		*/
		virtual ObjectTypeClass const & Class_Of(void) const;
		bool Class_Is_Valid(void) const override {
			if (!Class.Is_Valid() || Class.Raw() <= 0) return false;
			const UnitTypeClass* t = Class;
			if (!t) return false;
			if (t->RTTI != RTTI_UNITTYPE || t->MaxStrength <= 0) return false;

			/*
			**	Project Aeloria — TEMPORARY BAND-AID (see Is_Plausible_Class_Pointer in OBJECT.H
			**	and the full "Expert Panel Review" section in plan.md for rationale).
			**	This replaces the weaker range check that let 0x80738000 and similar through.
			**
			**	Tactical relaxation for visibility checkpoint: if the declared Class member
			**	looks plausible (Raw >0 and sane), trust it even if the runtime +8 peek is bad.
			**	This lets early-created units pass Is_Drawable / reach real shape data for
			**	client registration.
			*/
			uintptr_t effective = *(uintptr_t*)((const char*)this + 8);
			if (!Is_Plausible_Class_Pointer(effective)) {
				// Safe direct output (Aeloria_Debug_Log not visible from this header)
				char _buf[160];
				sprintf(_buf, "REJECTED Class_Is_Valid this=%p RTTI=%d Class.Raw=%d at+8=0x%08X\r\n",
				        this, What_Am_I(), Class.Raw(), (unsigned)effective);
				OutputDebugStringA(_buf);
				// CCPtr::Raw() is a heap ID — trust declared Class when dereferenced RTTI is sane.
				return true;
			}

			return true;
		}
		static void Init(void);

		bool Goto_Clear_Spot(void);
		bool Try_To_Deploy(void);
		virtual void Scatter(COORDINATE threat, bool forced=false, bool nokidding=false);

		int Tiberium_Check(CELL &center, int x, int y);
		bool Flag_Attach(HousesType house);
		bool Flag_Remove(void);
		bool Goto_Tiberium(int radius);
		bool Harvesting(void);

		/*
		**	Project Aeloria (Moderate cleanup):
		**	Extracted helper to find the best refinery for a harvester.
		*/
		BuildingClass * FindBestRefinery(bool friendly = false) const;

		/*
		**	Project Aeloria (Moderate cleanup):
		**	Centralized way to remember which refinery a harvester last used.
		**	This improves consistency across Mission_Harvest, Mission_Guard, and Mission_Repair.
		*/
		void RememberDockingBay(BuildingClass* refinery);

		/*
		**	Project Aeloria (Moderate cleanup):
		**	Harvester memory system helpers.
		**	These wrap the general ArchiveTarget field for harvester-specific "last harvest patch" memory.
		*/
		void RememberLastHarvestLocation(CELL cell);
		void ClearLastHarvestLocation();
		TARGET GetLastHarvestLocation() const;
		void APC_Close_Door(void);
		void APC_Open_Door(void);

		unsigned int Apply_Temporary_Jamming_Shroud(HouseClass *house_to_apply_for);
		void Unapply_Temporary_Jamming_Shroud(HouseClass *house_to_unapply_for, unsigned int shroud_bits_applied);

		/*
		**	Query functions.
		*/
		bool Should_Crush_It(TechnoClass const * it) const;
		int Credit_Load(void) const;
		virtual DirType Turret_Facing(void) const {if (Class->IsTurretEquipped) return(SecondaryFacing.Current());return(PrimaryFacing.Current());}
		int Shape_Number(void) const;
		virtual int Pip_Count(void) const;
		virtual InfantryType Crew_Type(void) const;
		virtual DirType Fire_Direction(void) const;
		virtual bool Ok_To_Move(DirType facing) const;
		virtual FireErrorType Can_Fire(TARGET target, int which) const;
		virtual fixed Tiberium_Load(void) const;

		/*
		**	Coordinate inquiry functions. These are used for both display and
		**	combat purposes.
		*/
		virtual COORDINATE Sort_Y(void) const;

		/*
		**	Object entry and exit from the game system.
		*/
		virtual bool Limbo(void);
		virtual bool Unlimbo(COORDINATE , DirType facing=DIR_N);

		/*
		**	Display and rendering support functionality. Supports imagery and how
		**	object interacts with the map and thus indirectly controls rendering.
		*/
		virtual short const * Overlap_List(bool redraw=false) const;
		virtual void Draw_It(int x, int y, WindowNumberType window) const;

		/*
		**	User I/O.
		*/
		virtual ActionType What_Action(CELL cell) const;
		virtual ActionType What_Action(ObjectClass const * object) const;
		virtual void Active_Click_With(ActionType action, ObjectClass * object);
		virtual void Active_Click_With(ActionType action, CELL cell);
		virtual void Player_Assign_Mission(MissionType mission, TARGET target, TARGET destination);

		/*
		**	Combat related.
		*/
		virtual ResultType Take_Damage(int & damage, int distance, WarheadType warhead, TechnoClass * source=0, bool forced=false);
		virtual BulletClass * Fire_At(TARGET target, int which=0);

		/*
		**	Driver control support functions. These are used to control cell
		**	occupation flags and driver instructions.
		*/
		virtual bool Start_Driver(COORDINATE & coord);

		/*
		**	AI.
		*/
		virtual TARGET Greatest_Threat(ThreatType threat) const;
		virtual DirType Desired_Load_Dir(ObjectClass * passenger, CELL & moveto) const;
		virtual RadioMessageType Receive_Message(RadioClass * from, RadioMessageType message, long & param);
		virtual void AI(void);
		virtual int Mission_Guard_Area(void);
		virtual int Mission_Unload(void);
		virtual int Mission_Guard(void);
		virtual int Mission_Harvest(void);
		virtual int Mission_Hunt(void);
		virtual int Mission_Repair(void);
		virtual int Mission_Move(void);
		virtual int Mission_Enter(void);
		void Rotation_AI(void);
		void Firing_AI(void);
		void Reload_AI(void);
		bool Edge_Of_World_AI(void);

		/*
		**	Scenario and debug support.
		*/
		#ifdef CHEAT_KEYS
		virtual void Debug_Dump(MonoClass *mono) const;
		#endif

		/*
		**	Movement and animation.
		*/
		virtual void Assign_Destination(TARGET target);
		virtual void Overrun_Square(CELL cell, bool threaten=true);
		virtual void Approach_Target(void);
		virtual int Offload_Tiberium_Bail(void);
		virtual void Enter_Idle_Mode(bool initial=false);
		virtual MoveType Can_Enter_Cell(CELL cell, FacingType facing=FACING_NONE) const;
		virtual void Per_Cell_Process(PCPType why);
		void Exit_Repair(void);
		void Shroud_Regen(void);

		/*
		**	File I/O.
		*/
		static void Read_INI(CCINIClass & ini);
		static void Write_INI(CCINIClass & ini);
		static char *INI_Name(void) {return "UNITS";};
		bool Load(Straw & file);
		bool Save(Pipe & file) const;
};

#endif