//
// Licensed to Green Energy Corp (www.greenenergycorp.com) under one
// or more contributor license agreements. See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  Green Enery Corp licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.
//

#ifndef __PHYS_LAYER_MONITOR_STATES_H_
#define __PHYS_LAYER_MONITOR_STATES_H_

#include "IPhysicalLayerObserver.h"
#include "Singleton.h"
#include "PhysicalLayerMonitor.h"

#define MACRO_MONITOR_SINGLETON(type, state) \
	MACRO_NAME_SINGLETON_INSTANCE(type) \
	PhysicalLayerState GetState() const { return state; }

namespace apl
{

class PhysicalLayerMonitor;

/* --- Base classes --- */

class IMonitorState
{
public:

	virtual void OnStartRequest(PhysicalLayerMonitor* apContext) = 0;
	virtual void OnCloseRequest(PhysicalLayerMonitor* apContext) = 0;
	virtual void OnSuspendRequest(PhysicalLayerMonitor* apContext) = 0;
	virtual void OnShutdownRequest(PhysicalLayerMonitor* apContext) = 0;
	
	virtual void OnOpenTimeout(PhysicalLayerMonitor* apContext) = 0;
	virtual void OnOpenFailure(PhysicalLayerMonitor* apContext) = 0;
	virtual void OnLayerOpen(PhysicalLayerMonitor* apContext) = 0;
	virtual void OnLayerClose(PhysicalLayerMonitor* apContext) = 0;

	virtual PhysicalLayerState GetState() const = 0;
	virtual std::string Name() const = 0;

	std::string ConvertToString();
};

class MonitorStateActions
{
public:

	static void ChangeState(PhysicalLayerMonitor* apContext, IMonitorState* apState);
	static void StartOpenTimer(PhysicalLayerMonitor* apContext);
	static void CancelOpenTimer(PhysicalLayerMonitor* apContext);	
	static void AsyncClose(PhysicalLayerMonitor* apContext);
	static void AsyncOpen(PhysicalLayerMonitor* apContext);
};

class ExceptsOnLayerOpen : public virtual IMonitorState
{
public:
	void OnLayerOpen(PhysicalLayerMonitor* apContext);
};

class NotOpening : public ExceptsOnLayerOpen
{
public:	
	void OnOpenFailure(PhysicalLayerMonitor* apContext);
};

class NotOpen : public virtual IMonitorState
{
public:
	void OnLayerClose(PhysicalLayerMonitor* apContext);
};

class NotWaitingForTimer : public virtual IMonitorState
{
public:
	void OnOpenTimeout(PhysicalLayerMonitor* apContext);
};

class IgnoresClose : public virtual IMonitorState
{
public:
	void OnCloseRequest(PhysicalLayerMonitor* apContext);
};

class IgnoresSuspend : public virtual IMonitorState
{
public:
	void OnSuspendRequest(PhysicalLayerMonitor* apContext);
};

class StartsOnClose : public virtual IMonitorState
{
public:
	void OnLayerClose(PhysicalLayerMonitor* apContext);
};

class IgnoresShutdown : public virtual IMonitorState
{
public:
	void OnShutdownRequest(PhysicalLayerMonitor* apContext);
};

class IgnoresStart : public virtual IMonitorState
{
public:
	void OnStartRequest(PhysicalLayerMonitor* apContext);
};

class OpenFailureCausesWait : public virtual IMonitorState
{
public:
	void OnOpenFailure(PhysicalLayerMonitor* apContext);
};

template <class T>
class OpenFailureGoesToState : public virtual IMonitorState
{
public:
	void OnOpenFailure(PhysicalLayerMonitor* apContext);
};



/* --- Concrete classes --- */

// disable "inherits via dominance warning", it's erroneous b/c base
// class is pure virtual and G++ correctly deduces this and doesn't care
#ifdef WIN32
#pragma warning(push)
#pragma warning(disable:4250)
#endif

class MonitorStateShutdown : public virtual IMonitorState,
	private NotOpening, 
	private NotOpen, 
	private NotWaitingForTimer, 
	private IgnoresClose, 
	private IgnoresStart, 
	private IgnoresShutdown,
	private IgnoresSuspend
{
	MACRO_MONITOR_SINGLETON(MonitorStateShutdown, PLS_SHUTDOWN);
};

class MonitorStateSuspended : public virtual IMonitorState,
	private NotOpening, private NotOpen, private NotWaitingForTimer, private IgnoresClose, private IgnoresSuspend
{
	MACRO_MONITOR_SINGLETON(MonitorStateSuspended, PLS_CLOSED);

	void OnStartRequest(PhysicalLayerMonitor* apContext);
	void OnShutdownRequest(PhysicalLayerMonitor* apContext);
};

class MonitorStateOpening : public virtual IMonitorState,
	private NotOpen, 
	private NotWaitingForTimer, 
	private OpenFailureCausesWait,
	private IgnoresStart
{
	MACRO_MONITOR_SINGLETON(MonitorStateOpening, PLS_OPENING);

	void OnCloseRequest(PhysicalLayerMonitor* apContext);
	void OnShutdownRequest(PhysicalLayerMonitor* apContext);	
	void OnSuspendRequest(PhysicalLayerMonitor* apContext);
	void OnLayerOpen(PhysicalLayerMonitor* apContext);
};

class MonitorStateOpeningClosing : public virtual IMonitorState,
	private NotOpen, 
	private NotWaitingForTimer,
	private ExceptsOnLayerOpen,
	private OpenFailureCausesWait, 
	private IgnoresStart, 
	private IgnoresClose
{
	MACRO_MONITOR_SINGLETON(MonitorStateOpeningClosing, PLS_OPENING);
	
	void OnShutdownRequest(PhysicalLayerMonitor* apContext);	
	void OnSuspendRequest(PhysicalLayerMonitor* apContext);
};

class MonitorStateOpeningStopping : public virtual IMonitorState,
	private NotOpen, 
	private NotWaitingForTimer, 
	private ExceptsOnLayerOpen,
	private OpenFailureGoesToState<MonitorStateShutdown>, 
	private IgnoresStart, 
	private IgnoresClose,
	private IgnoresSuspend,
	private IgnoresShutdown
{
	MACRO_MONITOR_SINGLETON(MonitorStateOpeningStopping, PLS_OPENING);	
};

class MonitorStateOpeningSuspending : public virtual IMonitorState,
	private NotOpen, 
	private NotWaitingForTimer, 
	private ExceptsOnLayerOpen,
	private OpenFailureGoesToState<MonitorStateSuspended>, 	
	private IgnoresClose,
	private IgnoresSuspend
{
	MACRO_MONITOR_SINGLETON(MonitorStateOpeningSuspending, PLS_OPENING);	

	void OnStartRequest(PhysicalLayerMonitor* apContext);
	void OnShutdownRequest(PhysicalLayerMonitor* apContext);
};

class MonitorStateOpen : public virtual IMonitorState,
	private NotOpening, private NotWaitingForTimer, private IgnoresStart, private StartsOnClose
{
	MACRO_MONITOR_SINGLETON(MonitorStateOpen, PLS_OPEN);

	void OnCloseRequest(PhysicalLayerMonitor* apContext);
	void OnSuspendRequest(PhysicalLayerMonitor* apContext);
	void OnShutdownRequest(PhysicalLayerMonitor* apContext);
};

class MonitorStateWaiting : public virtual IMonitorState,
	private NotOpening, private NotOpen, private IgnoresStart, private IgnoresClose
{
	MACRO_MONITOR_SINGLETON(MonitorStateWaiting, PLS_WAITING);

	void OnSuspendRequest(PhysicalLayerMonitor* apContext);	
	void OnShutdownRequest(PhysicalLayerMonitor* apContext);
	void OnOpenTimeout(PhysicalLayerMonitor* apContext);
};

class MonitorStateClosing : public virtual IMonitorState,
	private NotOpening, private NotWaitingForTimer, private IgnoresStart, private IgnoresClose, private StartsOnClose
{
	MACRO_MONITOR_SINGLETON(MonitorStateClosing, PLS_CLOSED);

	void OnSuspendRequest(PhysicalLayerMonitor* apContext);
	void OnShutdownRequest(PhysicalLayerMonitor* apContext);
};

class MonitorStateSuspending : public virtual IMonitorState,
	private NotOpening, private NotWaitingForTimer, private IgnoresClose, private IgnoresSuspend
{
	MACRO_MONITOR_SINGLETON(MonitorStateSuspending, PLS_CLOSED);

	void OnLayerClose(PhysicalLayerMonitor* apContext);
	void OnStartRequest(PhysicalLayerMonitor* apContext);
	void OnShutdownRequest(PhysicalLayerMonitor* apContext);
};

class MonitorStateShutingDown : public virtual IMonitorState,
	private NotOpening, 
	private NotWaitingForTimer, 
	private IgnoresStart, 
	private IgnoresClose,
	private IgnoresShutdown,
	private IgnoresSuspend
{
	MACRO_MONITOR_SINGLETON(MonitorStateShutingDown, PLS_CLOSED);

	void OnLayerClose(PhysicalLayerMonitor* apContext);
};

template <class T>
void OpenFailureGoesToState<T>::OnOpenFailure(PhysicalLayerMonitor* apContext)
{		
	MonitorStateActions::ChangeState(apContext, T::Inst());
}

#ifdef WIN32
#pragma warning(pop)
#endif

}

#endif

