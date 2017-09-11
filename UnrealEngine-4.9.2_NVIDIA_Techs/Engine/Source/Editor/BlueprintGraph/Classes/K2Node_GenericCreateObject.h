// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "K2Node.h"
#include "K2Node_ConstructObjectFromClass.h"
#include "K2Node_GenericCreateObject.generated.h"

UCLASS()
class BLUEPRINTGRAPH_API UK2Node_GenericCreateObject : public UK2Node_ConstructObjectFromClass
{
	GENERATED_BODY()

	// Begin UEdGraphNode interface.
	virtual void ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;
	virtual void ValidateNodeDuringCompilation(class FCompilerResultsLog& MessageLog) const override;
	virtual void EarlyValidation(class FCompilerResultsLog& MessageLog) const override;
	virtual bool IsCompatibleWithGraph(const UEdGraph* TargetGraph) const override;
	// End UEdGraphNode interface.

	virtual bool UseWorldContext() const override { return false; }
	virtual bool UseOuter() const override { return true; }
};
