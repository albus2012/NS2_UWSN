//
// Copyright (c) 1997 by the University of Southern California
// All rights reserved.
//
// Permission to use, copy, modify, and distribute this software and its
// documentation in source and binary forms for non-commercial purposes
// and without fee is hereby granted, provided that the above copyright
// notice appear in all copies and that both the copyright notice and
// this permission notice appear in supporting documentation. and that
// any documentation, advertising materials, and other materials related
// to such distribution and use acknowledge that the software was
// developed by the University of Southern California, Information
// Sciences Institute.  The name of the University may not be used to
// endorse or promote products derived from this software without
// specific prior written permission.
//
// THE UNIVERSITY OF SOUTHERN CALIFORNIA makes no representations about
// the suitability of this software for any purpose.  THIS SOFTWARE IS
// PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES,
// INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Other copyrights might apply to parts of this software and are so
// noted when applicable.
//
// $Header: /cvsroot/nsnam/nam-1/tag.h,v 1.3 2001/07/30 22:16:43 mehringe Exp $

#ifndef nam_tag_h
#define nam_tag_h

#include <tcl.h>
#include "animation.h"

class EditView;

class Tag : public Animation {
public:
	Tag();
	Tag(const char *name);
	virtual ~Tag();

	void setname(const char *name);
	inline const char *name() const { return name_; }
	virtual void move(EditView *v, float wdx, float wdy);

	void add(Animation *);
	void remove(void);
	void remove(Animation *);
	void update_bb(); // Recompute bbox after a member leaves

	virtual void reset(double now);
	virtual void draw(View *, double now);
	virtual void xdraw(View *v, double now) const; // xor-draw
	virtual const char *info() const;

	inline int classid() const { return ClassTagID; }
	inline int isMember(unsigned int id) const {
		return (Tcl_FindHashEntry(mbrHash_, (const char *)id) != NULL);
	}
	inline int isMember(Animation *a) const {
		return isMember(a->id());
	}

	void addNodeMovementDestination(EditView * editview, 
	                                double dx, double dy, double current_time);
	int getNodeMovementTimes(char * buffer, int buffer_size);
	
protected:
	Tcl_HashTable *mbrHash_;
	unsigned int nMbr_;
	char *name_;
};

#endif // nam_tag_h
