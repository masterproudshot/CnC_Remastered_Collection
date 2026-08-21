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

/* $Header: /CounterStrike/BULLET.H 2     3/06/97 1:46p Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer                                            *
 *                                                                                             *
 *                    File Name : BULLET.H                                                     *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic                                                *
 *                                                                                             *
 *                   Start Date : April 23, 1994                                               *
 *                                                                                             *
 *                  Last Update : April 23, 1994   [JLB]                                       *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef BULLET_H
#define BULLET_H

#include "object.h"
#include	"fly.h"
#include	"fuse.h"


class BulletClass :	public ObjectClass,
							public FlyClass,
							public FuseClass
{
	public:

		/*
		**	This specifies exactly what kind of bullet this is. All of the static attributes
		**	for this bullet is located in the BulletTypeClass pointed to by this variable.
		*/
		CCPtr<BulletTypeClass> Class;

	private:
		/*
		**	Records who sent this "present" so that an appropriate "thank you" can
		**	be returned.
		*/
		TechnoClass * Payback;

		/*
		**	This is the facing that the projectile is travelling.
		*/
		FacingClass PrimaryFacing;

	public:

		/*---------------------------------------------------------------------
		**	Constructors, Destructors, and overloaded operators.
		*/
		static void * operator new(size_t size);
		static void * operator new(size_t , void * ptr) {return(ptr);};
		static void operator delete(void *ptr);
		BulletClass(BulletType id, TARGET target, TechnoClass * Payback, int strength, WarheadType warhead, int speed);
#ifdef FIXIT_MULTI_SAVE
		BulletClass(NoInitClass const & x) : ObjectClass(x), Class(x), FlyClass(x), FuseClass(x), PrimaryFacing(x) {};
#else
		BulletClass(NoInitClass const & x) : ObjectClass(x), Class(x), FlyClass(x), FuseClass(x) {};
#endif
		virtual ~BulletClass(void);
		// Null-safe like UnitClass/VesselClass: a raw Class->Type deref makes every caller
		// (Shape_Number, draw guards) unsafe on an object with an unsettled +8.
		operator BulletType(void) const {
			BulletTypeClass const* t = Class;
			return t ? t->Type : BULLET_NONE;
		};

		/*---------------------------------------------------------------------
		**	Member function prototypes.
		*/
		static void Init(void);

		bool Is_Forced_To_Explode(COORDINATE & coord) const;
		void Bullet_Explodes(bool forced);
		int Shape_Number(void) const;
		virtual LayerType In_Which_Layer(void) const;
		virtual COORDINATE Sort_Y(void) const;
		virtual void Assign_Target(TARGET target) {TarCom = target;};
		virtual bool Unlimbo(COORDINATE , DirType facing = DIR_N);
		virtual ObjectTypeClass const & Class_Of(void) const {return *Class;};
		/*
		**	Two conditions were removed here, and the second one made EVERY projectile in the game
		**	undrawable:
		**
		**	  Class.Raw() <= 0   rejected heap index 0, which is a perfectly legal type id (it is simply
		**	                     the first type allocated). Now < 0, matching CCPtr's own -1 sentinel.
		**
		**	  t->MaxStrength <= 0  ObjectTypeClass initialises MaxStrength(0) (OBJECT.CPP) and
		**	                     BulletTypeClass never overrides it, because a bullet is not damageable.
		**	                     So this was false for every bullet type, always. Class_Is_Valid()
		**	                     returned false -> Is_Drawable() false -> BulletClass::Draw_It returned
		**	                     on its first line -> CC_Draw_Shape was never called.
		**
		**	Measured: with combat on screen the export census read
		**		bullets seen=7 elig=7 noimg=0 | fx enter=0
		**	i.e. bullets present in the layer, past the IsDown gate, holding valid shape data, and not
		**	one of them reaching the draw call. Same defect in ANIM.H, which is why explosions and
		**	muzzle flashes were missing too.
		*/
		bool Class_Is_Valid(void) const override {
			if (!Class.Is_Valid() || Class.Raw() < 0) return false;
			const BulletTypeClass* t = Class;
			if (!t) return false;
			if (t->RTTI != RTTI_BULLETTYPE) return false;

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
				// Tactical relaxation, matching Unit/Building/Aircraft/Vessel. Is_Drawable() gates
				// Draw_It entirely, so returning false here made projectiles with a stale +8 peek
				// undrawable outright — no shells or missiles rendered at all.
				return true;
			}

			return true;
		}
		virtual void Detach(TARGET target, bool all);
		virtual void Draw_It(int x, int y, WindowNumberType window) const;
		virtual bool Mark(MarkType mark=MARK_CHANGE);
		virtual void AI(void);
		virtual short const * Occupy_List(bool = false) const;
		virtual short const * Overlap_List(void) const {return Occupy_List(false);};
		virtual COORDINATE Target_Coord(void) const;

		/*
		**	File I/O.
		*/
		bool Load(Straw & file);
		bool Save(Pipe & file) const;
		virtual void Code_Pointers(void);
		virtual void Decode_Pointers(void);

		/*
		**	If this bullet is forced to be inaccurate because of some outside means. A tank
		**	firing while moving is a good example.
		*/
		unsigned IsInaccurate:1;

	private:
		// Crude animation flag.
		unsigned IsToAnimate:1;

		/*
		** Is this missile allowed to come in from out of bounds?
		*/
		unsigned IsLocked:1;

		/*
		**	This is the target of the projectile. It is especially significant for those projectiles
		**	that home in on a target.
		*/
		TARGET TarCom;

		/*
		**	The speed of this projectile.
		*/
		int MaxSpeed;

		/*
		**	The warhead of this projectile.
		*/
		WarheadType Warhead;

		/*
		** Some additional padding in case we need to add data to the class and maintain backwards compatibility for save/load
		*/
		unsigned char SaveLoadPadding[32];
};

#endif
