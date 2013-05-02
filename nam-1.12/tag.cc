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
// $Header: /cvsroot/nsnam/nam-1/tag.cc,v 1.6 2001/07/30 22:16:43 mehringe Exp $

#ifdef WIN32
#include <windows.h>
#endif
#include <string.h>
#include <tcl.h>
#include "animation.h"
#include "tag.h"
#include "view.h"
#include "node.h"
#include "editview.h"

Tag::Tag()
	: Animation(0, 0), nMbr_(0), name_(NULL)
{
	mbrHash_ = new Tcl_HashTable;
	Tcl_InitHashTable(mbrHash_, TCL_ONE_WORD_KEYS);
}

Tag::Tag(const char *name)
	: Animation(0, 0), nMbr_(0)
{
	size_t len = strlen(name);
	if (len == 0) 
		name_ = NULL;
	else {
		name_ = new char[len+1];
		strcpy(name_, name);
	}
	mbrHash_ = new Tcl_HashTable;
	Tcl_InitHashTable(mbrHash_, TCL_ONE_WORD_KEYS);
}

Tag::~Tag()
{
	remove();
	Tcl_DeleteHashTable(mbrHash_);
	delete mbrHash_;	
	if (name_ != NULL)
		delete []name_;
}

void Tag::setname(const char *name)
{
	if (name_ != NULL)
		delete name_;
	size_t len = strlen(name);
	if (len == 0) 
		name_ = NULL;
	else {
		name_ = new char[len+1];
		strcpy(name_, name);
	}
}

void Tag::add(Animation *a)
{
	int newEntry = 1;
	Tcl_HashEntry *he = 
		Tcl_CreateHashEntry(mbrHash_,(const char *)a->id(), &newEntry);
	if (newEntry && (he != NULL)) {
		// Do not handle exception he == NULL. Don't know how.
		Tcl_SetHashValue(he, (ClientData)a);
		nMbr_++;
		a->merge(bb_);
		// Let that object know about this group
		a->addTag(this); 
	}
}

void Tag::remove()
{
	// Remove this tag from all its members
	Animation *p;
	Tcl_HashEntry *he;
	Tcl_HashSearch hs;
	int i = 0;
	for (he = Tcl_FirstHashEntry(mbrHash_, &hs);
	     he != NULL;
	     he = Tcl_NextHashEntry(&hs), i++) {
		p = (Animation *) Tcl_GetHashValue(he);
		p->deleteTag(this);
		Tcl_DeleteHashEntry(he);
		nMbr_--;
	}
	bb_.clear();
}

void Tag::remove(Animation *a)
{
	Tcl_HashEntry *he = Tcl_FindHashEntry(mbrHash_, (const char *)a->id());
	if (he != NULL) {
		a->deleteTag(this);
		Tcl_DeleteHashEntry(he);
		nMbr_--;
		update_bb();
	}
}

void Tag::draw(View *v, double now) {
	// XXX don't draw anything except it's selected. Or draw 
	// 4 black dots in the 4 corners.
	v->rect(bb_.xmin, bb_.ymin, bb_.xmax, bb_.ymax, 
		Paint::instance()->thin());
}

// Used for xor-draw only
void Tag::xdraw(View *v, double now) const 
{
	// Draw every object of this group
	Tcl_HashEntry *he;
	Tcl_HashSearch hs;
	Animation *a;

	for (he = Tcl_FirstHashEntry(mbrHash_, &hs);
	     he != NULL;
	     he = Tcl_NextHashEntry(&hs)) {
		a = (Animation *) Tcl_GetHashValue(he);
		a->draw(v, now);
	}
}

void Tag::update_bb(void)
{
	Tcl_HashEntry *he;
	Tcl_HashSearch hs;
	Animation *a;

	bb_.clear();
	for (he = Tcl_FirstHashEntry(mbrHash_, &hs);
	     he != NULL;
	     he = Tcl_NextHashEntry(&hs)) {
		a = (Animation *) Tcl_GetHashValue(he);
		a->merge(bb_);
	}
}

void Tag::reset(double /*now*/)
{
}

//----------------------------------------------------------------------
// void 
// Tag::move(EditView *v, float wdx, float wdy)
//   -- Move all tagged object and update the bounding box
//----------------------------------------------------------------------
void
Tag::move(EditView *v, float wdx, float wdy) {
	Tcl_HashEntry *he;
	Tcl_HashSearch hs;
	Animation *a;

	bb_.clear();
	for (he = Tcl_FirstHashEntry(mbrHash_, &hs);
	     he != NULL;
	     he = Tcl_NextHashEntry(&hs)) {
		a = (Animation *) Tcl_GetHashValue(he);
		a->move(v, wdx, wdy);

		a->merge(bb_);
	}
}

const char *Tag::info() const
{
	static char str[256];
	strcpy(str, "Tag");
	return str;
}

//----------------------------------------------------------------------
// void
// Tag::addNodeMovementElements(EditView * editview, 
//                              double x, double y, double current_time)
//   - Used by the nam editor
//   - If any nodes in the tagged selection are mobile then the
//----------------------------------------------------------------------
void
Tag::addNodeMovementDestination(EditView * editview, 
                                double dx, double dy, double current_time) {
	Tcl_HashEntry * hash_entry;
	Tcl_HashSearch hash_search;
	Animation * animation_object;
	Node * node;
	float x,y;


	for (hash_entry = Tcl_FirstHashEntry(mbrHash_, &hash_search);
	     hash_entry != NULL;
	     hash_entry = Tcl_NextHashEntry(&hash_search)) {
		animation_object = (Animation *) Tcl_GetHashValue(hash_entry);

		if (animation_object->classid() == ClassNodeID) {
			node = (Node *) animation_object;
			x = node->x();
			y = node->y();

			editview->map(x,y);
			x += dx;
			y += dy;
			editview->imap(x,y);

			node->addMovementDestination(x, y, current_time);

		}
	}
}


//----------------------------------------------------------------------
// int
// Tag::getNodeMovementTimes(char * buffer, int buffer_size)
//   - returns a string of the form
//     {{<node id> <time> <time_2>...} {node_id2 <time> <time2>...}...}
//----------------------------------------------------------------------
int
Tag::getNodeMovementTimes(char * buffer, int buffer_size) {
	Tcl_HashEntry * hash_entry;
	Tcl_HashSearch hash_search;
	Animation * animation_object;
	Node * node;
	int chars_written, just_written;

	chars_written = 0;
	for (hash_entry = Tcl_FirstHashEntry(mbrHash_, &hash_search);
	     hash_entry != NULL;
	     hash_entry = Tcl_NextHashEntry(&hash_search)) {

		animation_object = (Animation *) Tcl_GetHashValue(hash_entry);

		if (animation_object->classid() == ClassNodeID) {
			node = (Node *) animation_object;
			if (chars_written  < buffer_size) {
				just_written = sprintf(buffer + chars_written, "{%d ", node->number());
				if (just_written != -1) {
					chars_written += just_written;
					chars_written += node->getMovementTimeList(buffer + chars_written,
					                                           buffer_size - chars_written) -1;
					chars_written += sprintf(buffer + chars_written, "}");
				} else {
					chars_written = -1;
					break;
				}
			}
		}
		chars_written += sprintf(buffer + chars_written, " ");
	}
	return chars_written;
}
