/*
 * Copyright (c) 1997 by the University of Southern California
 * All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation in source and binary forms for non-commercial purposes
 * and without fee is hereby granted, provided that the above copyright
 * notice appear in all copies and that both the copyright notice and
 * this permission notice appear in supporting documentation. and that
 * any documentation, advertising materials, and other materials related
 * to such distribution and use acknowledge that the software was
 * developed by the University of Southern California, Information
 * Sciences Institute.  The name of the University may not be used to
 * endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THE UNIVERSITY OF SOUTHERN CALIFORNIA makes no representations about
 * the suitability of this software for any purpose.  THIS SOFTWARE IS
 * PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * Other copyrights might apply to parts of this software and are so
 * noted when applicable.
 *
 * $Header: /cvsroot/nsnam/nam-1/group.cc,v 1.3 2001/04/18 00:14:15 mehringe Exp $
 */

#include <string.h>
#include "group.h"

Group::Group(const char *name, unsigned int addr) :
	Animation(0, 0), size_(0), addr_(addr), name_(0)
{
	if (name != NULL) 
		if (*name != 0) {
			name_ = new char[strlen(name)+1];
			strcpy(name_, name);
		}
	nodeHash_ = new Tcl_HashTable;
	Tcl_InitHashTable(nodeHash_, TCL_ONE_WORD_KEYS);
}

Group::~Group()
{
	if (name_ != NULL)
		delete name_;
	Tcl_DeleteHashTable(nodeHash_);
	delete nodeHash_;
}

int Group::join(int id)
{
	int newEntry = 1;
	Tcl_HashEntry *he = Tcl_CreateHashEntry(nodeHash_, (const char *)id, 
						&newEntry);
	if (he == NULL)
		return -1;
	if (newEntry) {
		Tcl_SetHashValue(he, (ClientData)id);
		size_++;
	}
	return 0;
}

void Group::leave(int id)
{
	Tcl_HashEntry *he = Tcl_FindHashEntry(nodeHash_, (const char *)id);
	if (he != NULL) {
		Tcl_DeleteHashEntry(he);
		size_--;
	}
}

// Assume mbrs has at least size_ elements
void Group::get_members(int *mbrs)
{
	Tcl_HashEntry *he;
	Tcl_HashSearch hs;
	int i = 0;
	for (he = Tcl_FirstHashEntry(nodeHash_, &hs);
	     he != NULL;
	     he = Tcl_NextHashEntry(&hs), i++) 
		mbrs[i] = (int) Tcl_GetHashValue(he);
}

void Group::draw(View * nv, double now) {
	// Do nothing for now. Will add group visualization later.
}
