// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ModuleInterface.h"


class IPortalRpcLocator;
class IPortalRpcResponder;


/**
 * Interface for the PortalRpc module.
 */
class IPortalRpcModule
	: public IModuleInterface
{
public:

	virtual TSharedRef<IPortalRpcLocator> CreateLocator() = 0;
	virtual TSharedRef<IPortalRpcResponder> CreateResponder() = 0;
};
