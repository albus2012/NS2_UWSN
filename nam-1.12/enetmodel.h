//
// Copyright (c) 2000 by the University of Southern California
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
// $Header: /cvsroot/nsnam/nam-1/enetmodel.h,v 1.18 2003/10/11 22:56:50 xuanc Exp $
//
// Network model for Editor

#ifndef nam_enetmodel_h
#define nam_enetmodel_h

#include "netmodel.h"
#include "trafficsource.h"
#include "lossmodel.h"
#include "queuehandle.h"

// The various classifications of agents and traffic generators
// used for determining compatibility (so users can't link incompatible
// objects together in the editor)
typedef enum {
	UDP_SOURCE_AGENT, UDP_SINK_AGENT,
	TCP_SOURCE_AGENT, TCP_SINK_AGENT,
	FULLTCP_AGENT,
	UNKNOWN_AGENT
	} agent_type;

typedef enum {
	UDP_APP,
	TCP_APP,
	FULLTCP_APP,
	UNKNOWN_APP
	} trafgen_type;

// The ratio of node radius and mean edge length in the topology

class EditorNetModel : public NetModel {
public:
	EditorNetModel(const char *animator);
	virtual ~EditorNetModel();
	void BoundingBox(BBox&);

	Node * addNode(Node * node);
	Node * addNode(int node_id);
	int addNode(float cx, float cy);
	Edge* addEdge(Edge * edge);
	int addLink(Node*, Node*);
	int addLink(int source_id, int destination_id);

	agent_type classifyAgent(const char * agent);
	bool checkAgentCompatibility(agent_type source, agent_type dest);
	bool checkAgentCompatibility(const char * source, const char * dest);
	int addAgent(Agent * agent, Node * node);
	int addAgent(Node * src, const char * agent_type, float cx, float cy);
	int addAgentLink(Agent*, Agent*);
	int addAgentLink(int source_id, int destination_id);

	trafgen_type classifyTrafficSource(const char * source);
	bool checkAgentTrafficSourceCompatibility(agent_type agent, trafgen_type source);
	bool checkAgentTrafficSourceCompatibility(const char * agent, const char * source);
	int addTrafficSource(TrafficSource * traffic_source, Agent * agent);
	int addTrafficSource(Agent * agent, const char * type, float cx, float cy);
	TrafficSource * lookupTrafficSource(int id);

	int addLossModel(LossModel * loss_model, Edge * edge);
	int addLossModel(Edge * edge, const char * type, double cx, double cy);
	LossModel * lookupLossModel(int id);

	QueueHandle * addQueueHandle(Edge * edge);
	QueueHandle * addQueueHandle(Edge * edge, const char * type);
	int writeNsScript(FILE * file, const char * filename, const char * filename_root);

	void setNodeProperty(int id, const char * value, const char* variable);
	void setAgentProperty(int id, const char* value, const char* variable);
	void setLinkProperty(int source_id, int destination_id,
	                     const char * value, const char * variable);
	void setQueueHandleProperty(int source_id, int destination_id,
	                            const char * value, const char * variable);
	void setTrafficSourceProperty(int id, const char* value, const char* variable);
	void setLossModelProperty(int id, const char* value, const char * variable);
	void layout();

	void removeLink(Edge *);
	void removeNode(Node *);
	void removeAgent(Agent *a);
	void removeTrafficSource(TrafficSource * ts);
	void removeLossModel(LossModel * lossmodel);

	inline EditorNetModel* wobj() { return wobj_; }

protected:
	EditorNetModel *wobj_;
	int command(int argc, const char*const* argv);


private:
	int node_id_count;
	double	pxmin_;
	double	pymin_;
	double	pxmax_;
	double	pymax_;

	TrafficSource * traffic_sources_;
	LossModel * lossmodels_;
	QueueHandle * queue_handles_;

	double maximum_simulation_time_; //seconds
	double playback_speed_; //microseconds (us)

};


#endif // nam_enetmodel_h
