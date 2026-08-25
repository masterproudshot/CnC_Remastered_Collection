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

/* $Header: /CounterStrike/VESSEL.H 1     3/03/97 10:26a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer                                            *
 *                                                                                             *
 *                    File Name : VESSEL.H                                                     *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic                                                *
 *                                                                                             *
 *                   Start Date : 03/13/96                                                     *
 *                                                                                             *
 *                  Last Update : March 13, 1996 [JLB]                                         *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef VESSEL_H
#define VESSEL_H

#include "drive.h"
#include "radio.h"
#include "cargo.h"
#include "mission.h"
#include	"target.h"

class VesselClass : public DriveClass
{
	public:
		/*
		**	This points to the controling static characteristic data associated with
		**	this vessel.
		*/
		CCPtr<VesselTypeClass> Class;

		/*
		** Has this sea vessel been told to move to a shipyard?  If so, then
		** when we get there, start the repair process.
		*/
		unsigned IsToSelfRepair:1;

		/*
		** Is this sea vessel parked next to a shipyard/subpen, and therefore
		** in the special self-repair mode?
		*/
		unsigned IsSelfRepairing:1;

		/*
		** If this is an LST, is it time to shut the door?
		*/
		CDTimerClass<FrameTimerClass> DoorShutCountDown;

		/*
		** If this is a sub, has the sonar pulse worn off, such that we can
		** re-submerge?
		*/
		CDTimerClass<FrameTimerClass> PulseCountDown;

		VesselClass(VesselType classid, HousesType house);
		VesselClass(NoInitClass const & x) : DriveClass(x), Class(x), SecondaryFacing(x) {};
		static void * operator new(size_t size);
		static void * operator new(size_t , void * ptr) {return(ptr);};
		static void operator delete(void *ptr);
		// Null-safe like UnitClass::operator UnitType (UNIT.H:149). The raw Class->Type deref made
		// VesselClass::Shape_Number() unsafe for an untrusted vessel, which forced the caller onto a
		// shape-0 fallback — frame 0 is FACING NORTH, so the hull pointed north no matter which way
		// the ship sailed while its turret tracked correctly.
		operator VesselType(void) const {
			VesselTypeClass const* t = Class;
			return t ? t->Type : VESSEL_NONE;
		};

		static void Init(void);

		virtual ~VesselClass(void);
		virtual ObjectTypeClass const & Class_Of(void) const;
		bool Class_Is_Valid(void) const override {
			if (!Class.Is_Valid() || Class.Raw() < 0) return false;
			const VesselTypeClass* t = Class;
			if (!t) return false;
			if (t->RTTI != RTTI_VESSELTYPE) return false;

			/*
			**	Project Aeloria — TEMPORARY BAND-AID (see Is_Plausible_Class_Pointer in OBJECT.H
			**	and the full "Expert Panel Review" section in plan.md for rationale).
			**	This replaces the weaker range check that let 0x80738000 and similar through.
			*/
			uintptr_t effective = *(uintptr_t*)((const char*)this + 8);
			if (!Is_Plausible_Class_Pointer(effective)) {
				// Safe direct output (Aeloria_Debug_Log not visible from this header)
				char _buf[160];
				sprintf(_buf, "REJECTED Class_Is_Valid this=%p RTTI=%d Class.Raw=%d at+8=0x%08X\r\n",
				        this, What_Am_I(), Class.Raw(), (unsigned)effective);
				OutputDebugStringA(_buf);
				// Tactical relaxation, matching UnitClass and BuildingClass — see the note in AIRCRAFT.H.
				// Is_Drawable() gates Draw_It entirely, so a false here silently removed the vessel from
				// the tactical view and the radar together.
				return true;
			}

			return true;
		}

		virtual MZoneType Zone_Check_Type(void) const {return(MZONE_WATER);}
		int Shape_Number(void) const;
		void Rotation_AI(void);
		void Combat_AI(void);
		bool Edge_Of_World_AI(void);
		void Repair_AI(void);
		virtual DirType Turret_Facing(void) const {if (Class->IsTurretEquipped) return(SecondaryFacing.Current());return(PrimaryFacing.Current());}
		virtual bool Start_Driver(COORDINATE & headto);
		virtual int Mission_Retreat(void);
		virtual int Mission_Unload(void);
		void LST_Open_Door(void);
		void LST_Close_Door(void);
		virtual FireDataType Fire_Data(int) const;
		virtual COORDINATE Fire_Coord(int which) const;
		virtual MoveType Can_Enter_Cell(CELL cell, FacingType from=FACING_NONE) const;
		virtual void Draw_It(int x, int y, WindowNumberType window) const;
		virtual short const * Overlap_List(bool redraw=false) const;
		virtual DirType Desired_Load_Dir(ObjectClass * passenger, CELL & moveto) const;
		virtual RadioMessageType Receive_Message(RadioClass * from, RadioMessageType message, long & param);
		virtual void AI(void);
		virtual void Per_Cell_Process(PCPType why);
		virtual void Assign_Destination(TARGET target);
		virtual int Pip_Count(void) const;

		virtual ResultType Take_Damage(int & damage, int distance, WarheadType warhead, TechnoClass * source=0, bool forced=false);
		virtual FireErrorType Can_Fire(TARGET target, int which) const;

		virtual void Enter_Idle_Mode(bool initial=false);
		virtual ActionType What_Action(ObjectClass const * object) const;
		virtual ActionType What_Action(CELL cell) const;
		virtual void Active_Click_With(ActionType action, CELL cell);
		virtual void Active_Click_With(ActionType action, ObjectClass * object);
		virtual TARGET Greatest_Threat(ThreatType threat) const;
		virtual bool Is_Allowed_To_Recloak(void) const;
#ifdef FIXIT_CARRIER	//	checked - ajw 9/28/98
		virtual BulletClass * Fire_At(TARGET target, int which=0);
#endif
		/*
		**	File I/O.
		*/
		static void Read_INI(CCINIClass & ini);
		static void Write_INI(CCINIClass & ini);
		static char * INI_Name(void) {return "SHIPS";};
		bool Load(Straw & file);
		bool Save(Pipe & file) const;

		/*
		**	Scenario and debug support.
		*/
		#ifdef CHEAT_KEYS
		virtual void Debug_Dump(MonoClass *mono) const;
		#endif

	protected:
		/*
		**	This is the facing of the turret. It can be, and usually is,
		**	rotated independently of the body it is attached to.
		*/
		FacingClass SecondaryFacing;

		/*
		** Some additional padding in case we need to add data to the class and maintain backwards compatibility for save/load
		*/
		unsigned char SaveLoadPadding[32];

};

#endif